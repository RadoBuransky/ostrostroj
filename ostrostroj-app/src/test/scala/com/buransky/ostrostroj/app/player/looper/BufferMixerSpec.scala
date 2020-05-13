package com.buransky.ostrostroj.app.player.looper

import java.nio.ByteBuffer

import com.buransky.ostrostroj.app.common.OstrostrojException
import javax.sound.sampled.AudioFormat
import org.junit.runner.RunWith
import org.scalatest.flatspec.AnyFlatSpec
import org.scalatestplus.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
class BufferMixerSpec extends AnyFlatSpec {
  private val audioFormat = new AudioFormat(44100, 16, 2, true, false)

  behavior of "xfadingBufferLength"

  it should "compute the right thing" in {
    // Execute
    val result = BufferMixer.xfadingBufferLength(audioFormat)

    // Assert
    assert(result == 352)
  }

  behavior of "mix"

  it should "fail if format is not supported" in {
    val ex = intercept[OstrostrojException] {
      val bb = ByteBuffer.allocate(1)
      BufferMixer.mix(new AudioFormat(44100, 8, 2, true, false), bb, bb, 1.0, 1.0, bb)
    }
    assert(ex.getMessage.startsWith("Unsupported mixing format!"))
  }

  it should "fail if track buffers don't have the same starting positions" in {
    val ex = intercept[OstrostrojException] {
      val track1 = ByteBuffer.allocate(2)
      track1.position(1)
      val track2 = ByteBuffer.allocate(2)
      val dst = ByteBuffer.allocate(1)
      BufferMixer.mix(audioFormat, track1, track2, 1.0, 1.0, dst)
    }
    assert(ex.getMessage.startsWith("Tracks for mixing are different!"))
  }

  it should "fail if track buffers don't have the same size" in {
    val ex = intercept[OstrostrojException] {
      val track1 = ByteBuffer.allocate(1)
      val track2 = ByteBuffer.allocate(2)
      val dst = ByteBuffer.allocate(1)
      BufferMixer.mix(audioFormat, track1, track2, 1.0, 1.0, dst)
    }
    assert(ex.getMessage.startsWith("Tracks for mixing are different!"))
  }

  it should "fail if track buffers are larger than the destination buffer" in {
    val ex = intercept[OstrostrojException] {
      val track1 = ByteBuffer.allocate(2)
      val track2 = ByteBuffer.allocate(2)
      val dst = ByteBuffer.allocate(1)
      BufferMixer.mix(audioFormat, track1, track2, 1.0, 1.0, dst)
    }
    assert(ex.getMessage.startsWith("Not enough room to mix into!"))
  }

  it should "work for empty buffers" in {
    // Prepare
    val track1 = ByteBuffer.allocate(0)
    val track2 = ByteBuffer.allocate(0)
    val dst = ByteBuffer.allocate(0)

    // Execute
    val result = BufferMixer.mix(audioFormat, track1, track2, 1.0, 1.0, dst)

    // Assert
    assert(result == 0)
  }

  it should "mix one sample 0:0" in {
    // Prepare
    val track1 = ByteBuffer.allocate(2)
    val track2 = ByteBuffer.allocate(2)
    val dst = ByteBuffer.allocate(2)
    track1.put(0x12.toByte)
    track1.put(0x34.toByte)
    track1.clear()
    track2.put(0x56.toByte)
    track2.put(0x78.toByte)
    track2.clear()

    // Execute
    val result = BufferMixer.mix(audioFormat, track1, track2, 0.0, 0.0, dst)

    // Assert
    assert(result == 2)
    dst.clear()
    assert(dst.get() == 0x00)
    assert(dst.get() == 0x00)
  }

  it should "mix one sample 1:1" in {
    // Prepare
    val track1 = ByteBuffer.allocate(2)
    val track2 = ByteBuffer.allocate(2)
    val dst = ByteBuffer.allocate(2)
    track1.put(0x01.toByte)
    track1.put(0x00.toByte)
    track1.clear()
    track2.put(0x02.toByte)
    track2.put(0x00.toByte)
    track2.clear()

    // Execute
    val result = BufferMixer.mix(audioFormat, track1, track2, 1.0, 1.0, dst)

    // Assert
    assert(result == 2)
    dst.clear()
    assert(dst.get() == 0x03)
    assert(dst.get() == 0x00)
  }

  it should "mix one sample 0.5:0.0" in {
    // Prepare
    val track1 = ByteBuffer.allocate(2)
    val track2 = ByteBuffer.allocate(2)
    val dst = ByteBuffer.allocate(2)
    track1.put(0x00.toByte)
    track1.put(0x0A.toByte)
    track1.clear()
    track2.put(0x00.toByte)
    track2.put(0x00.toByte)
    track2.clear()

    // Execute
    val result = BufferMixer.mix(audioFormat, track1, track2, 0.5, 0.0, dst)

    // Assert
    assert(result == 2)
    dst.clear()
    assert(dst.get() == 0x12)
    assert(dst.get() == 0x07)
  }

  it should "mix one sample 0.3:0.7" in {
    // Prepare
    val track1 = ByteBuffer.allocate(2)
    val track2 = ByteBuffer.allocate(2)
    val dst = ByteBuffer.allocate(2)
    track1.put(0x00.toByte)
    track1.put(0x10.toByte)
    track1.clear()
    track2.put(0x00.toByte)
    track2.put(0x10.toByte)
    track2.clear()

    // Execute
    val result = BufferMixer.mix(audioFormat, track1, track2, 0.3, 0.7, dst)

    // Assert
    assert(result == 2)
    dst.clear()
    assert(dst.get() == 0x26)
    assert(dst.get() == 0x16)
  }

  behavior of "xfade"

  it should "fail if format is not supported" in {
    val ex = intercept[OstrostrojException] {
      val bb = ByteBuffer.allocate(1)
      BufferMixer.xfade(new AudioFormat(44100, 8, 2, true, false), bb, bb)
    }
    assert(ex.getMessage.startsWith("Unsupported crossfading format!"))
  }

  it should "fail if to buffer is smaller than from buffer" in {
    val ex = intercept[OstrostrojException] {
      val from = ByteBuffer.allocate(2)
      val to = ByteBuffer.allocate(1)
      BufferMixer.xfade(audioFormat, from, to)
    }
    assert(ex.getMessage.startsWith("Not enough room to crossfade!"))
  }

  it should "crossfade" in {
    // Prepare
    val from = ByteBuffer.allocate(10)
    val to = ByteBuffer.allocate(20)
    from.putShort(0x0001)
    from.putShort(0x0001)
    from.putShort(0x0001)
    from.putShort(0x0001)
    from.putShort(0x0001)
    from.clear()
    to.putShort(0x0002)
    to.putShort(0x0002)
    to.putShort(0x0002)
    to.putShort(0x0002)
    to.putShort(0x0002)
    to.putShort(0x0002)
    to.putShort(0x0002)
    to.putShort(0x0002)
    to.putShort(0x0002)
    to.putShort(0x0002)
    to.clear()

    // Execute
    val result = BufferMixer.xfade(audioFormat, from, to)

    // Assert
    assert(result == 10)
    to.clear()
    assert(to.getShort() == 0x0001)
    assert(to.getShort() == 0xC901.toShort)
    assert(to.getShort() == 0x0A02)
    assert(to.getShort() == 0x2E02)
    assert(to.getShort() == 0x3C02)
    assert(to.getShort() == 0x0002)
    assert(to.getShort() == 0x0002)
    assert(to.getShort() == 0x0002)
    assert(to.getShort() == 0x0002)
    assert(to.getShort() == 0x0002)
  }
}
