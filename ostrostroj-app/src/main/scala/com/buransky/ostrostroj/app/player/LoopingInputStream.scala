package com.buransky.ostrostroj.app.player

import java.io.{InputStream, RandomAccessFile}
import java.nio.file.Path

import javax.sound.sampled.AudioFormat

class LoopingInputStream(path: Path, audioFormat: AudioFormat) extends InputStream {
  import LoopingInputStream._
  private val file = new RandomAccessFile(path.toFile, "r")
  private val crossfader = new Crossfader((audioFormat.getSampleRate*(audioFormat.getSampleSizeInBits/8)*
    audioFormat.getChannels*crossfadeLengthMs.toDouble/1000.0).toInt, audioFormat)

  def startLooping(start: Long, end: Long): Unit = ???
  def stopLooping(): Unit = ???

  override def read(): Int = ???
  override def close(): Unit = {
    super.close()
    file.close()
  }
}

object LoopingInputStream {
  private val crossfadeLengthMs = 50
}