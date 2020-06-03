package com.buransky.ostrostroj.app.audio.impl

import com.buransky.ostrostroj.app.audio.{AudioBuffer, AudioMixerChannel, FrameCount}
import javax.sound.sampled.AudioFormat
import org.junit.runner.RunWith
import org.scalatest.flatspec.AnyFlatSpec
import org.scalatestplus.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
class AudioMixerImplSpec extends AnyFlatSpec {
  private val audioFormat = new AudioFormat(44100, 16, 2, true, false)
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
    val dst = AudioBuffer(audioFormat, 4)
    val channel1 = AudioMixerChannel(1.0, AudioBuffer(audioFormat, 8).copy(limit = FrameCount(2)))
    val channel2 = AudioMixerChannel(1.0, AudioBuffer(audioFormat, 8).copy(limit = FrameCount(2)))

    // Execute
    val ex = intercept[IllegalArgumentException] {
      mixer.mix(List(channel1, channel2), dst)
    }
    assert(ex.getMessage == "Buffer is small.")
  }
}
