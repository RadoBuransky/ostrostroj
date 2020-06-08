package com.buransky.ostrostroj.app.audio.impl.input

import com.buransky.ostrostroj.app.audio._
import com.buransky.ostrostroj.app.show.{Loop, Track}
import com.google.common.base.Preconditions._
import org.slf4j.LoggerFactory

private[audio] case class LoadedTrack(track: Track, audioBuffer: AudioBuffer)

private[audio] class LoopInputImpl(loop: Loop,
                                   loadedTracks: Seq[LoadedTrack],
                                   startingPosition: FrameCount,
                                   audioMixer: AudioMixer) extends LoopInput {
  import LoopInputImpl._

  checkArgument(startingPosition.value >= loop.start, (s"Position outside of loop! [${startingPosition.value}, " +
    s"${loop.start}]").asInstanceOf[AnyRef])
  checkArgument(startingPosition.value < loop.endExclusive, (s"Position outside of loop! [${startingPosition.value}, " +
    s"${loop.endExclusive}]").asInstanceOf[AnyRef])
  checkArgument(loop.tracks.nonEmpty, "At least one track is needed!".asInstanceOf[AnyRef])
  checkArgument(loadedTracks.map(_.track).toSet == loop.tracks.toSet, "Not all tracks loaded!".asInstanceOf[AnyRef])

  private var level = 0
  private val minLevel = loop.tracks.map(_.rangeMin).min
  private val maxLevel = loop.tracks.map(_.rangeMax).max
  private var isDraining = false
  private var songPosition: FrameCount = startingPosition

  override def harder(): Int = synchronized {
    if (level < maxLevel) {
      level += 1
    }
    logger.debug(s"Harder. [$level, $maxLevel]")
    level
  }

  override def softer(): Int = synchronized {
    if (level > minLevel) {
      level -= 1
    }
    logger.debug(s"Softer. [$level, $minLevel]")
    level
  }

  override def read(buffer: AudioBuffer): AudioBuffer = synchronized {
    logger.trace(s"Read. [${buffer.position}, ${buffer.limit}]")
    if (songPosition.value == loop.endExclusive) {
      if (isDraining) {
        logger.debug(s"Draining done. [$songPosition, ${loop.endExclusive}]")
        buffer.copy(position = FrameCount(0), limit = FrameCount(0), endOfStream = true)
      }
      else {
        logger.debug(s"Next loop.")
        songPosition = FrameCount(loop.start)
        safeRead(buffer)
      }
    } else {
      safeRead(buffer)
    }
  }

  override def close(): Unit = {}

  override def startDraining(): Unit = synchronized {
    isDraining = true
  }

  override def stopDraining(): Unit = synchronized {
    isDraining = false
  }

  override def toggleDraining(): Unit = synchronized {
    isDraining = !isDraining
  }

  private def safeRead(buffer: AudioBuffer): AudioBuffer = {
    val channels = loadedTracks
      .filter(t => isInRange(level, t.track))
      .map(trackToMixerChannel(level, _))
      .map(limitChannelView(songPosition, buffer.capacity, _))
    val mixedResult = audioMixer.mix(channels, buffer)
    songPosition += mixedResult.size
    if (songPosition.value > loop.endExclusive) {
      songPosition = FrameCount(loop.endExclusive)
    }
    mixedResult
  }

  private def isInRange(level: Int, track: Track): Boolean = track.channelLevel(level) != 0

  private def limitChannelView(songPosition: FrameCount, bufferCapacity: FrameCount,
                               mixerChannel: AudioMixerChannel): AudioMixerChannel = {
    val viewPosition = songPosition - FrameCount(loop.start)
    val viewLimit = FrameCount(Math.min(songPosition.value + bufferCapacity.value, loop.endExclusive)) -
      FrameCount(loop.start)
    val limitedAudioBuffer = mixerChannel.audioBuffer.copy(position = viewPosition, limit = viewLimit)
    AudioMixerChannel(mixerChannel.level, limitedAudioBuffer)
  }

  private def trackToMixerChannel(level: Int, track: LoadedTrack): AudioMixerChannel =
    AudioMixerChannel(track.track.channelLevel(level), track.audioBuffer)

  override def status: LoopStatus = LoopStatus(
    loop = loop,
    level = level,
    minLevel = minLevel,
    maxLevel = maxLevel,
    isDraining = isDraining,
    position = songPosition,
    done = isDraining && songPosition.value == loop.endExclusive)
}

private object LoopInputImpl {
  private val logger = LoggerFactory.getLogger(classOf[LoopInputImpl])
}