package com.buransky.ostrostroj.app.audio

import com.buransky.ostrostroj.app.audio.impl.AudioMixerImpl
import javax.sound.sampled.AudioFormat

private[audio] case class AudioMixerChannel(level: Double, audioBuffer: AudioBuffer) {
  def readLeShort(frame: FrameCount, channel: Int): AudioMixerSample = {
    val sample = audioBuffer.readLeShort(audioBuffer.position + frame, channel)
    AudioMixerSample(level, sample)
  }
}
private[audio] case class AudioMixerSample(level: Double, sample: Short)

private[audio] trait AudioMixer {
  def mix(audioMixerChannels: Iterable[AudioMixerChannel], dst: AudioBuffer): AudioBuffer
}

private[audio] object AudioMixer {
  def apply(audioFormat: AudioFormat): AudioMixer = new AudioMixerImpl(audioFormat)
}