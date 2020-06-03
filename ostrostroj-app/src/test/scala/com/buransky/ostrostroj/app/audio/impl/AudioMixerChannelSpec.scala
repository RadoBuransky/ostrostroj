package com.buransky.ostrostroj.app.audio.impl

import com.buransky.ostrostroj.app.audio.{AudioBuffer, AudioMixerChannel, FrameCount}
import javax.sound.sampled.AudioFormat
import org.junit.runner.RunWith
import org.scalatest.flatspec.AnyFlatSpec
import org.scalatestplus.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
class AudioMixerChannelSpec extends AnyFlatSpec {
  behavior of "readLeShort"

  it should "read 16-bit little endian sample" in {
    // Prepare
    val audioFormat = new AudioFormat(44100, 16, 2, true, false)
    val audioBuffer = AudioBuffer(audioFormat, 8)
    audioBuffer.byteArray.update(4, 0x1A)
    audioBuffer.byteArray.update(5, 0x2B)
    val audioMixerChannel = AudioMixerChannel(0.7, audioBuffer)

    // Execute
    val result = audioMixerChannel.readLeShort(FrameCount(1), channel = 0)

    // Assert
    assert(result.level == 0.7)
    assert(result.sample == 0x2B1A)
  }
}
