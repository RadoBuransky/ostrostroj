package com.buransky.ostrostroj.app.audio.impl

import com.buransky.ostrostroj.app.audio._
import javax.sound.sampled.{Mixer, SourceDataLine}
import org.slf4j.LoggerFactory

private[audio] class OstrostrojPlayer(mixer: Mixer,
                                      sourceDataLine: SourceDataLine,
                                      audioOutput: AudioOutput,
                                      playlistInput: PlaylistInput,
                                      audioProvider: AudioProvider) extends AudioPlayer {
  import OstrostrojPlayer._

  override def play(): Unit = audioOutput.start()
  override def pause(): Unit = audioOutput.stop()
  override def toggleLooping(): Unit = playlistInput.songInput.toggleLooping()
  override def harder(): Option[Int] = playlistInput.songInput.loopInput.map(_.harder())
  override def softer(): Option[Int] = playlistInput.songInput.loopInput.map(_.softer())
  override def volumeUp(): Double = audioOutput.volumeUp()
  override def volumeDown(): Double = audioOutput.volumeDown()
  override def status: AudioPlayerStatus = {
    val playlistStatus = playlistInput.status
    AudioPlayerStatus(
      song = playlistStatus.songStatus.song,
      position = playlistStatus.songStatus.position,
      volume = audioOutput.volume,
      done = playlistStatus.done && audioOutput.framesBuffered.value == 0,
      isPlaying = sourceDataLine.isRunning,
      looping = playlistStatus.songStatus.loopStatus.map { loopStatus =>
        AudioPlayerLoopingStatus(
          loop = loopStatus.loop,
          level = loopStatus.level,
          minLevel = loopStatus.minLevel,
          maxLevel = loopStatus.maxLevel,
          isDraining = loopStatus.isDraining
        )
      }
    )
  }

  override def close(): Unit = {
    logger.debug("Closing Ostrostroj player...")
    sourceDataLine.drain()
    audioProvider.close()
    audioOutput.close()
    sourceDataLine.close()
    playlistInput.close()
    mixer.close()
    logger.info("Ostrostroj player closed.")
  }
}

private object OstrostrojPlayer {
  private val logger = LoggerFactory.getLogger(classOf[OstrostrojPlayer])
}