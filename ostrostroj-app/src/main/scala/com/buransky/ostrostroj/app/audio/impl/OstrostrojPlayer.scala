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
  override def harder(): Unit = playlistInput.songInput.loopInput.foreach(_.harder())
  override def softer(): Unit = playlistInput.songInput.loopInput.foreach(_.softer())
  override def volumeUp(): Unit = audioOutput.volumeUp()
  override def volumeDown(): Unit = audioOutput.volumeDown()
  override def status: AudioPlayerStatus = ???

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