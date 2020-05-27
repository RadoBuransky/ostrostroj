package com.buransky.ostrostroj.app.audio.impl

import com.buransky.ostrostroj.app.audio.{JavaSoundOutput, OstrostrojPlayer, OstrostrojPlayerStatus, PlaylistInput}
import com.buransky.ostrostroj.app.show.Playlist
import javax.sound.sampled.{AudioFormat, AudioSystem}
import org.slf4j.LoggerFactory

private[audio] class JavaSoundOstrostrojPlayer(playlist: Playlist) extends OstrostrojPlayer {
  private lazy val audioFormat: AudioFormat = AudioSystem.getAudioFileFormat(playlist.songs.head.path.toFile).getFormat
  private val javaSoundOutput = JavaSoundOutput(audioFormat)

  override def start(songIndex: Int): Unit = {
    PlaylistInput(songIndex, javaSoundOutput)
  }
  override def pause(): Unit = ???
  override def resume(): Unit = ???
  override def startLooping(): Unit = ???
  override def stopLooping(): Unit = ???
  override def harder(): Unit = ???
  override def softer(): Unit = ???
  override def setVolume(volume: Int): Unit = ???
  override def getStatus(): OstrostrojPlayerStatus = ???

  override def close(): Unit = {
    javaSoundOutput.close()
  }
}

private object JavaSoundOstrostrojPlayer {
  private val logger = LoggerFactory.getLogger(classOf[JavaSoundOstrostrojPlayer])
}