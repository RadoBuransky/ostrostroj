package com.buransky.ostrostroj.app.audio

private[audio] case class AudioMixerChannel(level: Double, audioBuffer: AudioBuffer)

private[audio] trait AudioMixer {
  def mix(channels: Iterable[AudioMixerChannel], dst: AudioBuffer): AudioBuffer
}

private[audio] object AudioMixer {
  def apply(): AudioMixer = ???
}