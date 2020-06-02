package com.buransky.ostrostroj.app.audio

import java.nio.ByteBuffer

final case class ByteCount(value: Int) extends AnyVal
final case class SampleCount(value: Int) extends AnyVal

final case class PlaybackPosition(songIndex: Int, position: SampleCount)

sealed trait AudioSample extends Any {
  def value: Int
}

/**
 * 16-bit little endian sample.
 * @param underlying Raw value as present in the audio stream.
 */
final class AudioSample16Le(val underlying: Short) extends AnyVal with AudioSample {
  def value: Int = ??? // TODO: Swap bytes I guess
}

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
  def sample(index: SampleCount, channel: Int): AudioSample = ???
}