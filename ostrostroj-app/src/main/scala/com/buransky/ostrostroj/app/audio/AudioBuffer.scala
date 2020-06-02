package com.buransky.ostrostroj.app.audio

import java.nio.ByteBuffer
import java.nio.file.Path

import javax.sound.sampled.spi.AudioFileReader

final case class ByteCount(value: Int) extends AnyVal
final case class SampleCount(value: Int) extends AnyVal

sealed trait BitsPerSample
case object SixteenBits extends BitsPerSample
case object EightBits extends BitsPerSample

/**
 * Wrapper for ByteBuffer containing audio data.
 */
class AudioBuffer(byteBuffer: ByteBuffer,
                  val channels: Int,
                  val bitsPerSample: BitsPerSample,
                  val position: SampleCount,
                  val limit: SampleCount,
                  val endOfStream: Boolean) {
  def byteArray: Array[Byte] = byteBuffer.array()
  def bytePosition: Int = ???
  def byteLimit: Int = ???
  def byteSize: Int = byteLimit - bytePosition
  def clear(): Unit = ???
  def size: SampleCount = ???
}

object AudioBuffer {
  def apply(path: Path, audioFileReader: AudioFileReader): AudioBuffer = {
    ???
  }
}