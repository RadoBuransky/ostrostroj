package com.buransky.ostrostroj.app.audio

import java.nio.ByteBuffer

final case class ByteCount(index: Int) extends AnyVal
final case class SampleCount(index: Int) extends AnyVal

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

sealed trait AudioBuffer {
  def raw: ByteBuffer

  def channels: Int
  def bitsPerSample: BitsPerSample

  def position: SampleCount
  def limit: SampleCount
  def capacity: SampleCount

  /**
   * Position of the first sample of this buffer within the audio stream.
   */
  def startPosition: PlaybackPosition

  /**
   * Position of the last sample of this buffer within the audio stream.
   */
  def endPosition: PlaybackPosition

  def clear(): Unit
  def sample(index: SampleCount, channel: Int): AudioSample
}

final class AudioBuffer16Le(val channels: Int,
                            val raw: ByteBuffer,
                            val startPosition: PlaybackPosition,
                            val endPosition: PlaybackPosition) extends AudioBuffer {
  override val bitsPerSample: BitsPerSample = SixteenBits
  override def position: SampleCount = ???
  override def limit: SampleCount = ???
  override def capacity: SampleCount = ???
  override def clear(): Unit = raw.clear()
  override def sample(index: SampleCount, channel: Int): AudioSample = ???
}
