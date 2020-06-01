package com.buransky.ostrostroj.app.audio.impl

import com.buransky.ostrostroj.app.audio.{OstrostrojPlayer, OstrostrojPlayerStatus}
import com.buransky.ostrostroj.app.show.Playlist
import javax.sound.sampled.{AudioFormat, AudioSystem}
import org.slf4j.LoggerFactory

private[audio] class JavaSoundOstrostrojPlayer(playlist: Playlist) extends OstrostrojPlayer {
  private lazy val audioFormat: AudioFormat = AudioSystem.getAudioFileFormat(playlist.songs.head.path.toFile).getFormat

  override def play(): Unit = ???
  override def pause(): Unit = ???
  override def startLooping(): Unit = ???
  override def stopLooping(): Unit = ???
  override def harder(): Unit = ???
  override def softer(): Unit = ???
  override def setVolume(volume: Int): Unit = ???
  override def getStatus(): OstrostrojPlayerStatus = ???

  override def close(): Unit = {
  }
}

private object JavaSoundOstrostrojPlayer {
  private val logger = LoggerFactory.getLogger(classOf[JavaSoundOstrostrojPlayer])
}