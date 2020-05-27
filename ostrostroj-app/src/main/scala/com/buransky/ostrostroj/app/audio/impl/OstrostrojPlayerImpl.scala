package com.buransky.ostrostroj.app.audio.impl

import com.buransky.ostrostroj.app.audio.{JavaSoundOutput, OstrostrojPlayer, OstrostrojPlayerStatus}
import com.buransky.ostrostroj.app.show.Playlist
import javax.sound.sampled.{AudioFormat, AudioSystem}
import org.slf4j.LoggerFactory

private[audio] class OstrostrojPlayerImpl(playlist: Playlist) extends OstrostrojPlayer {
  import OstrostrojPlayerImpl._

  private lazy val audioFormat: AudioFormat = AudioSystem.getAudioFileFormat(playlist.songs.head.path.toFile).getFormat
  private val javaSoundOutput = JavaSoundOutput(audioFormat)
  private var songIndex = 0

  override def selectSong(songIndex: Int): Unit = synchronized {
    logger.debug(s"Select song. [$songIndex, ${this.songIndex}]")
    if (songIndex != this.songIndex) {
      stop()
    }
    this.songIndex = songIndex
  }

  override def start(): Unit = ???

  override def stop(): Unit = ???

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

private object OstrostrojPlayerImpl {
  private val logger = LoggerFactory.getLogger(classOf[OstrostrojPlayerImpl])
}