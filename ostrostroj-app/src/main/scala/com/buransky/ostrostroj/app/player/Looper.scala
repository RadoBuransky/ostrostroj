package com.buransky.ostrostroj.app.player

import java.nio.ByteBuffer

import com.buransky.ostrostroj.app.show.Loop
import javax.sound.sampled.AudioInputStream

class Looper(loops: Seq[Loop], upstream: AudioInputStream) {
  private var position: Long = 0
  private val buffer: ByteBuffer = ???

  def startLooping(): Unit = ???
  def stopLooping(): Unit = ???

  def read(buffer: Array[Byte]): Int = {
    // TODO: Are we looping and is the whole loop already buffered? Then get data from the buffer

    val bytesRead = upstream.read(buffer)
    if (bytesRead > 0) {
      val bufferStart = position
      val bufferEndExcl = position + bytesRead
      position += bytesRead

      // TODO: Buffer data if inside a loop (+ some margin before loop start for crossfading)
      // TODO: Apply crossfade if this is the end of loop
    }

    bytesRead
  }
}

object Looper {
}