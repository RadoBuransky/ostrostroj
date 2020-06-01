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

/**
 * Wrapper for ByteBuffer containing audio data.
 */
class AudioBuffer(byteBuffer: ByteBuffer,
                  val channels: Int,
                  val bitsPerSample: BitsPerSample,
                  val position: SampleCount,
                  val limit: SampleCount,
                  val capacity: SampleCount,
                  val endOfStream: Boolean) {
  def byteArray: Array[Byte] = byteBuffer.array()
  def bytePosition: Int = ???
  def byteLimit: Int = ???
  def clear(): Unit = ???
  def sample(index: SampleCount, channel: Int): AudioSample = ???
}

/**
 * AudioBuffer with position within an audio stream.
 * @param startPosition Position of the first sample of this buffer within the audio stream.
 */
class AudioEvent(byteBuffer: ByteBuffer,
                 channels: Int,
                 bitsPerSample: BitsPerSample,
                 position: SampleCount,
                 limit: SampleCount,
                 capacity: SampleCount,
                 endOfStream: Boolean,
                 val startPosition: PlaybackPosition) extends AudioBuffer(byteBuffer, channels, bitsPerSample, position,
  limit, capacity, endOfStream) {
  /**
   * Position of the last sample of this buffer within the audio stream.
   */
  def endPosition: PlaybackPosition = PlaybackPosition(startPosition.songIndex,
    SampleCount(startPosition.position.index + limit.index - position.index))
}