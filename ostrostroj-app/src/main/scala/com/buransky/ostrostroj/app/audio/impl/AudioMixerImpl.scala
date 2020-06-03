package com.buransky.ostrostroj.app.audio.impl

import com.buransky.ostrostroj.app.audio.{AudioBuffer, AudioMixer, AudioMixerChannel, FrameCount}
import com.google.common.base.Preconditions._
import javax.sound.sampled.AudioFormat

private case class AudioMixerSample(level: Double, sample: Short)

private[audio] class AudioMixerImpl(audioFormat: AudioFormat) extends AudioMixer {
  override def mix(audioMixerChannels: Iterable[AudioMixerChannel], dst: AudioBuffer): AudioBuffer = {
    if (audioMixerChannels.nonEmpty) {
      checkArgs(audioMixerChannels, dst)
      val first = audioMixerChannels.head
      for (frame <- 0 until first.audioBuffer.size.value) {
        val frameCount = FrameCount(frame)
        for (channel <- 0 until audioFormat.getChannels) {
          val samples = audioMixerChannels.map(readLeShort(frameCount, channel, _))
          val mixedSample = mix16bitLeSamples(samples)
          dst.putLeShort(mixedSample, frameCount, channel)
        }
      }
      dst
    } else {
      dst
    }
  }

  private def mix16bitLeSamples(samples: Iterable[AudioMixerSample]): Short = {
    val sum = samples.map(s => s.level*s.sample).sum
    if (sum < Short.MinValue)
      Short.MinValue
    else {
      if (sum > Short.MaxValue)
        Short.MaxValue
      else
        sum.toShort
    }
  }

  private def readLeShort(frame: FrameCount, channel: Int, audioMixerChannel: AudioMixerChannel): AudioMixerSample = {
    val sample = audioMixerChannel.audioBuffer.readLeShort(audioMixerChannel.audioBuffer.position + frame, channel)
    AudioMixerSample(audioMixerChannel.level, sample)
  }

  private def checkArgs(channels: Iterable[AudioMixerChannel], dst: AudioBuffer): Unit = {
    val first = channels.head
    checkArgument(channels.forall(_.audioBuffer.position == first.audioBuffer.position))
    checkArgument(channels.forall(_.audioBuffer.limit == first.audioBuffer.limit))
    checkArgument(first.audioBuffer.size.value <= dst.size.value)
  }
}
