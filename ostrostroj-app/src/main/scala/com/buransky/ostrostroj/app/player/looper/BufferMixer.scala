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
  def mix16bitLe(track1: ByteBuffer, track2: ByteBuffer, track1Level: Float, track2Level: Float,
                 dst: ByteBuffer): Int = {
    if ((track1.position() != track2.position()) || (track1.limit() != track2.limit())) {
      throw new OstrostrojException(s"Tracks for mixing are different! [${track1.position()}, ${track2.position()}, " +
        s"${track1.limit()}, ${track2.limit()}]")
    }

    val srcSize = track1.limit() - track1.position()
    val dstSize = dst.limit() - dst.position()
    if (srcSize > dstSize) {
      throw new OstrostrojException(s"Not enough room to mix into! [$srcSize, $dstSize]")
    }

    while (track1.position() < track1.limit()) {
      val sample1 = readLeShort(track1)
      val sample2 = readLeShort(track2)
      val mixedSample = mix16bitLeSamples(sample1, sample2, track1Level, track2Level)
      putLeShort(mixedSample, dst)
    }

    srcSize
  }

  /**
   * Cross-fades 16-bit little-endian "from" track into "to" track.
   * @return Number of bytes cross-faded.
   */
  def cross16bitLe(from: ByteBuffer, to: ByteBuffer): Int = {
    val fromSize = from.limit() - from.position()
    val toSize = to.limit() - to.position()
    if (toSize < fromSize) {
      throw new OstrostrojException(s"Not enough room to crossfade! [$fromSize, $toSize]")
    }

    val startingPosition = from.position()
    while (from.position() < from.limit()) {
      val t = (from.position() - startingPosition).toDouble/fromSize.toDouble
      val fromSample = readLeShort(from)
      val toSample = readLeShort(to)
      val mixedSample = mix16bitLeSamples(fromSample, toSample, 1.0 - t, t)
      to.position(to.position() - 2)
      putLeShort(mixedSample, to)
    }

    fromSize
  }

  /**
   * https://dsp.stackexchange.com/questions/14754/equal-power-crossfade
   */
  private def mix16bitLeSamples(sample1: Short, sample2: Short, level1: Double, level2: Double): Short =
    (sample1*Math.sqrt(level1) + sample2*Math.sqrt(level2)).toShort

  private def putLeShort(s: Short, buffer: ByteBuffer): Unit = {
    val (loByte, hiByte) = shortToLeBytes(s)
    buffer.put(loByte)
    buffer.put(hiByte)
  }
  private def readLeShort(buffer: ByteBuffer): Short = {
    val loByte = buffer.get()
    val hiByte = buffer.get()
    leBytesToShort(loByte, hiByte)
  }
  private def shortToLeBytes(s: Short): (Byte, Byte) = ((s & 0xFF).toByte, ((s >>> 8) & 0xFF).toByte)
  private def leBytesToShort(loByte: Byte, hiByte: Byte): Short = (((hiByte & 0xFF) << 8) | (loByte & 0xFF)).toShort
}
