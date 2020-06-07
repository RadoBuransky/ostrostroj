package com.buransky.ostrostroj.app.audio.impl

import com.buransky.ostrostroj.app.audio._
import javax.sound.sampled.SourceDataLine
import org.slf4j.LoggerFactory

private[audio] class OstrostrojPlayer(sourceDataLine: SourceDataLine,
                                      audioOutput: AudioOutput,
                                      playlistInput: PlaylistInput,
                                      audioProvider: AudioProvider) extends AudioPlayer {
  import OstrostrojPlayer._

  override def play(): Unit = sourceDataLine.start()
  override def pause(): Unit = sourceDataLine.stop()
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
      isPaused = !sourceDataLine.isRunning,
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
    audioProvider.close()
    audioOutput.close()
    sourceDataLine.close()
    playlistInput.close()
    logger.info("Ostrostroj player closed.")
  }
}

private object OstrostrojPlayer {
  private val logger = LoggerFactory.getLogger(classOf[OstrostrojPlayer])
}