package com.buransky.ostrostroj.app.audio.impl.input

import com.buransky.ostrostroj.app.audio._
import com.buransky.ostrostroj.app.show.{Loop, Track}
import com.google.common.base.Preconditions._
import org.slf4j.LoggerFactory

private[audio] case class LoadedTrack(track: Track, audioBuffer: AudioBuffer)

private[audio] class LoopInputImpl(loop: Loop,
                                   tracks: Seq[LoadedTrack],
                                   startingPosition: FrameCount,
                                   audioMixer: AudioMixer) extends LoopInput {
  import LoopInputImpl._

  checkArgument(startingPosition.value >= loop.start)
  checkArgument(startingPosition.value < loop.endExclusive)

  private var level = 0
  private val minLevel = tracks.map(_.track.rangeMin).min
  private val maxLevel = tracks.map(_.track.rangeMax).max
  private var isDraining = false
  private var position: FrameCount = startingPosition

  override def harder(): Unit = synchronized {
    logger.debug(s"Harder. [$level, $maxLevel]")
    if (level < maxLevel) {
      level += 1
    }
  }

  override def softer(): Unit = synchronized {
    logger.debug(s"Softer. [$level, $minLevel]")
    if (level > minLevel) {
      level -= 1
    }
  }

  override def read(buffer: AudioBuffer): AudioBuffer = synchronized {
    logger.trace(s"Read. [${buffer.position}, ${buffer.limit}]")
    if (position.value == loop.endExclusive) {
      if (isDraining) {
        logger.debug(s"Draining done. [$position, ${loop.endExclusive}]")
        buffer.copy(position = FrameCount(0), limit = FrameCount(0), endOfStream = true)
      }
      else {
        logger.debug(s"Next loop.")
        position = FrameCount(0)
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
    val channels = tracks
      .filter(t => isInRange(level, t.track))
      .map(trackToMixerChannel(level, _))
      .map(limitChannelView(position, buffer.capacity, _))
    val mixedResult = audioMixer.mix(channels, buffer)
    position += mixedResult.size
    mixedResult
  }

  private def isInRange(level: Int, track: Track): Boolean =
    (level >= (track.rangeMin - track.fade)) && (level <= (track.rangeMax + track.fade))

  private def limitChannelView(position: FrameCount, bufferCapacity: FrameCount,
                               mixerChannel: AudioMixerChannel): AudioMixerChannel = {
    val viewPosition = position - FrameCount(loop.start)
    val viewLimit = FrameCount(Math.min(position.value + bufferCapacity.value, loop.endExclusive))
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
    position = position)
}

private object LoopInputImpl {
  private val logger = LoggerFactory.getLogger(classOf[LoopInputImpl])
}