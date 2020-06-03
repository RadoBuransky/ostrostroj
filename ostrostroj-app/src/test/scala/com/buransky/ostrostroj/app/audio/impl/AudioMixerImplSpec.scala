package com.buransky.ostrostroj.app.audio.impl

import com.buransky.ostrostroj.app.audio.{AudioBuffer, AudioMixerChannel, FrameCount}
import javax.sound.sampled.AudioFormat
import org.junit.runner.RunWith
import org.scalatest.flatspec.AnyFlatSpec
import org.scalatestplus.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
class AudioMixerImplSpec extends AnyFlatSpec {
  private val audioFormat = new AudioFormat(44100, 16, 1, true, false)
  private val mixer = new AudioMixerImpl(audioFormat)

  behavior of "mix"

  it should "do nothing for an empty collection of channels" in {
    // Prepare
    val dst = AudioBuffer(audioFormat, 4)

    // Execute
    val result = mixer.mix(Nil, dst)

    // Assert
    assert(result.byteArray.forall(_ == 0))
    assert(result == dst)
  }

  it should "fail if position of all channels is not the same" in {
    // Prepare
    val dst = AudioBuffer(audioFormat, 8)
    val channel1 = AudioMixerChannel(1.0, AudioBuffer(audioFormat, 8))
    val channel2 = AudioMixerChannel(1.0, AudioBuffer(audioFormat, 8).copy(position = FrameCount(1),
      limit = FrameCount(1)))

    // Execute
    val ex = intercept[IllegalArgumentException] {
      mixer.mix(List(channel1, channel2), dst)
    }
    assert(ex.getMessage == "Position is not the same.")
  }

  it should "fail if limit of all channels is not the same" in {
    // Prepare
    val dst = AudioBuffer(audioFormat, 8)
    val channel1 = AudioMixerChannel(1.0, AudioBuffer(audioFormat, 8))
    val channel2 = AudioMixerChannel(1.0, AudioBuffer(audioFormat, 8).copy(limit = FrameCount(1)))

    // Execute
    val ex = intercept[IllegalArgumentException] {
      mixer.mix(List(channel1, channel2), dst)
    }
    assert(ex.getMessage == "Limit is not the same.")
  }

  it should "fail if buffer is not big enough" in {
    // Prepare
    val dst = AudioBuffer(audioFormat, 2)
    val channel1 = AudioMixerChannel(1.0, AudioBuffer(audioFormat, 8).copy(limit = FrameCount(2)))
    val channel2 = AudioMixerChannel(1.0, AudioBuffer(audioFormat, 8).copy(limit = FrameCount(2)))

    // Execute
    val ex = intercept[IllegalArgumentException] {
      mixer.mix(List(channel1, channel2), dst)
    }
    assert(ex.getMessage == "Buffer is small. [4, 1]")
  }

  it should "sum two channels using saturated arithmetic" in {
    // Prepare
    val dst = AudioBuffer(audioFormat, 6)
    val channel1 = AudioMixerChannel(1.0, AudioBuffer(audioFormat, 16).copy(position = FrameCount(2),
      limit = FrameCount(5)))
    val channel2 = AudioMixerChannel(1.0, AudioBuffer(audioFormat, 16).copy(position = FrameCount(2),
      limit = FrameCount(5)))
    channel1.audioBuffer.putLeShort(100, FrameCount(2), 0)
    channel1.audioBuffer.putLeShort(Short.MaxValue, FrameCount(3), 0)
    channel1.audioBuffer.putLeShort(Short.MinValue, FrameCount(4), 0)
    channel2.audioBuffer.putLeShort(-200, FrameCount(2), 0)
    channel2.audioBuffer.putLeShort(1, FrameCount(3), 0)
    channel2.audioBuffer.putLeShort(-1, FrameCount(4), 0)

    // Execute
    val result = mixer.mix(List(channel1, channel2), dst)

    // Assert
    assert(result.position == FrameCount(0))
    assert(result.limit == FrameCount(3))
    assert(result.frameSize == dst.frameSize)
    assert(result.channels == dst.channels)
    assert(result.endOfStream == dst.endOfStream)
    assert(result.readLeShort(FrameCount(0), 0) == -100)
    assert(result.readLeShort(FrameCount(1), 0) == Short.MaxValue)
    assert(result.readLeShort(FrameCount(2), 0) == Short.MinValue)
  }

  it should "sum channels using levels" in {
    // Prepare
    val dst = AudioBuffer(audioFormat, 6)
    val channel1 = AudioMixerChannel(0.25, AudioBuffer(audioFormat, 16).copy(position = FrameCount(2),
      limit = FrameCount(5)))
    val channel2 = AudioMixerChannel(0.50, AudioBuffer(audioFormat, 16).copy(position = FrameCount(2),
      limit = FrameCount(5)))
    channel1.audioBuffer.putLeShort(100, FrameCount(2), 0)
    channel1.audioBuffer.putLeShort(Short.MaxValue, FrameCount(3), 0)
    channel1.audioBuffer.putLeShort(Short.MinValue, FrameCount(4), 0)
    channel2.audioBuffer.putLeShort(-200, FrameCount(2), 0)
    channel2.audioBuffer.putLeShort(Short.MaxValue, FrameCount(3), 0)
    channel2.audioBuffer.putLeShort(-1, FrameCount(4), 0)

    // Execute
    val result = mixer.mix(List(channel1, channel2), dst)

    // Assert
    assert(result.position == FrameCount(0))
    assert(result.limit == FrameCount(3))
    assert(result.frameSize == dst.frameSize)
    assert(result.channels == dst.channels)
    assert(result.endOfStream == dst.endOfStream)
    assert(result.readLeShort(FrameCount(0), 0) == -75)
    assert(result.readLeShort(FrameCount(1), 0) == 24575)
    assert(result.readLeShort(FrameCount(2), 0) == -8192)
  }
}
