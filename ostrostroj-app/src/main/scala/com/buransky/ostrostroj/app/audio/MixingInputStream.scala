package com.buransky.ostrostroj.app.audio

import java.io.{InputStream, RandomAccessFile}
import java.nio.ByteBuffer
import java.util

import javax.sound.sampled.AudioFormat

// TODO: Split reading from file and writing to mixed into independent Futures.
// TODO: Make sure all tracks are in sync when buffering.

class LoopingInputStream(track: RandomAccessFile, audioFormat: AudioFormat) extends InputStream {
  private val buffer = ByteBuffer.allocate(5*44100*2*2) // 5 second buffer for 44.1 kHz, 16 bit, stereo

  def loop(startPos: Long, endPos: Long): Unit = ???
  def stopLooping(): Unit = ???
  override def read(): Int = buffer.get()
}

class MutingInputStream(track: InputStream) extends InputStream {
  private var muted = false
  def mute(): Unit = {
    muted = true
  }
  def unmute(): Unit = {
    muted = false
  }
  override def read(): Int = {
    track.read()
    0
  }
  override def read(b: Array[Byte]): Int = {
    val result = track.read(b)
    if (muted) {
      util.Arrays.fill(b, 0.toByte)
    }
    result
  }
  override def read(b: Array[Byte], off: Int, len: Int): Int = {
    val result = track.read(b, off, len)
    if (muted) {
      util.Arrays.fill(b, off, off + len - 1, 0.toByte)
    }
    result
  }
  override def skip(n: Long): Long = track.skip(n)
  override def reset(): Unit = {}
  override def mark(readlimit: Int): Unit = {}
  override def markSupported(): Boolean = false
  override def close(): Unit = super.close()
}

class MixingInputStream(tracks: Iterable[InputStream], audioFormat: AudioFormat) extends InputStream {
  override def read(): Int = throw new UnsupportedOperationException()
  override def read(b: Array[Byte]): Int = {
    val buffer = new Array[Byte](b.length)
    tracks.map(_.read(buffer)).head
    // TODO: ...
  }
  override def read(b: Array[Byte], off: Int, len: Int): Int = ???
  override def skip(n: Long): Long = tracks.map(_.skip(n)).head
  override def reset(): Unit = {}
  override def mark(readlimit: Int): Unit = {}
  override def markSupported(): Boolean = false
  override def close(): Unit = super.close()
}
