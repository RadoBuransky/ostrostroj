package com.buransky.ostrostroj.app.player.looper

import java.nio.ByteBuffer
import java.time.Duration

import com.buransky.ostrostroj.app.common.OstrostrojException
import javax.sound.sampled.AudioFormat

object BufferMixer {
  val xfadingDuration: Duration = Duration.ofMillis(10)

  /**
   *
   * @return Buffer size in bytes.
   */
  def xfadingBufferLength(audioFormat: AudioFormat): Int = {
    import audioFormat._
    (xfadingDuration.toMillis*getSampleSizeInBits*getChannels*getSampleRate/(8*1000)).toInt
  }

  def mix(audioFormat: AudioFormat, track1: ByteBuffer, track2: ByteBuffer, track1Level: Float, track2Level: Float,
          dst: ByteBuffer): Int = {
    if (!audioFormat.isBigEndian && audioFormat.getSampleSizeInBits == 16) {
      mix16bitLe(track1, track2, track1Level, track2Level, dst)
    } else {
      throw new OstrostrojException(s"Unsupported mixing format!" +
        s"[${audioFormat.isBigEndian}, ${audioFormat.getSampleSizeInBits}]")
    }
  }

  /**
   * Cross-fades "from" track into "to" track.
   * @return Number of bytes cross-faded.
   */
  def xfade(audioFormat: AudioFormat, from: ByteBuffer, to: ByteBuffer): Int = {
    if (!audioFormat.isBigEndian && audioFormat.getSampleSizeInBits == 16) {
      cross16bitLe(from, to)
    } else {
      throw new OstrostrojException(s"Unsupported crossfading format!" +
        s"[${audioFormat.isBigEndian}, ${audioFormat.getSampleSizeInBits}]")
    }
  }

  /**
   * Mixes two 16-bit little-endian tracks into destination buffer.
   * @param track1 Track 1 audio data.
   * @param track2 Track 2 audio data.
   * @param track1Level Track 1 mixing level (0.0 - 1.0).
   * @param track2Level Track 2 mixing level (0.0 - 1.0).
   * @param dst Destination buffer.
   * @return Number of bytes actually mixed.
   */
  def mix16bitLe(track1: ByteBuffer, track2: ByteBuffer, track1Level: Float, track2Level: Float, dst: ByteBuffer): Int = ???


  /**
   * Cross-fades 16-bit little-endian "from" track into "to" track.
   * @return Number of bytes cross-faded.
   */
  def cross16bitLe(from: ByteBuffer, to: ByteBuffer): Int = ???
}
