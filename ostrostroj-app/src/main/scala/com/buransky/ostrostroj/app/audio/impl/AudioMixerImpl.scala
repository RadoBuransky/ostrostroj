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
          putLeShort(mixedSample, frameCount, channel, dst)
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

  private def putLeShort(s: Short, position: FrameCount, channel: Int, buffer: AudioBuffer): Unit = {
    val (loByte, hiByte) = shortToLeBytes(s)
    val samplePosition = frameChannelToSample(position, channel)
    buffer.byteArray.update(samplePosition, loByte)
    buffer.byteArray.update(samplePosition + 1, hiByte)
  }

  private def readLeShort(frame: FrameCount, channel: Int, audioMixerChannel: AudioMixerChannel): AudioMixerSample = {
    val sample = readLeShort(audioMixerChannel.audioBuffer.position + frame, channel, audioMixerChannel.audioBuffer)
    AudioMixerSample(audioMixerChannel.level, sample)
  }

  private def readLeShort(position: FrameCount, channel: Int, buffer: AudioBuffer): Short = {
    val samplePosition = frameChannelToSample(position, channel)
    val loByte = buffer.byteArray(samplePosition)
    val hiByte = buffer.byteArray(samplePosition + 1)
    leBytesToShort(loByte, hiByte)
  }

  private def shortToLeBytes(s: Short): (Byte, Byte) = ((s & 0xFF).toByte, ((s >>> 8) & 0xFF).toByte)

  private def leBytesToShort(loByte: Byte, hiByte: Byte): Short = (((hiByte & 0xFF) << 8) | (loByte & 0xFF)).toShort

  private def frameChannelToSample(position: FrameCount, channel: Int): Int =
    position.value*audioFormat.getFrameSize + channel*audioFormat.getSampleSizeInBits/8

  private def checkArgs(channels: Iterable[AudioMixerChannel], dst: AudioBuffer): Unit = {
    val first = channels.head
    checkArgument(channels.forall(_.audioBuffer.position == first.audioBuffer.position))
    checkArgument(channels.forall(_.audioBuffer.limit == first.audioBuffer.limit))
    checkArgument(first.audioBuffer.size.value <= dst.size.value)
  }
}
