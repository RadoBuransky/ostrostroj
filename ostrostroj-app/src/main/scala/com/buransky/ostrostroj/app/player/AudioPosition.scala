package com.buransky.ostrostroj.app.player

import java.time.Duration

import com.google.common.base.Preconditions
import javax.sound.sampled.AudioFormat

sealed trait AudioPosition {
  def audioFormat: AudioFormat
}
final case class TimePosition(audioFormat: AudioFormat, timePosition: Duration) extends AudioPosition {
  Preconditions.checkArgument(timePosition.toMillis >= 0, timePosition)
  def toByte: BytePosition = toSample.toByte
  def toSample: SamplePosition = {
    val unalignedPosition = (timePosition.toMillis.toDouble / (1000.0*audioFormat.getSampleRate)).toInt
    SamplePosition(audioFormat, unalignedPosition - (unalignedPosition % audioFormat.getFrameSize) )
  }
}
final case class BytePosition(audioFormat: AudioFormat, bytePosition: Int) extends AudioPosition {
  Preconditions.checkArgument(bytePosition >= 0, bytePosition)
  Preconditions.checkArgument(bytePosition % audioFormat.getFrameSize == 0, bytePosition)
  def toTime: TimePosition = toSample.toTime
  def toSample: SamplePosition =
    SamplePosition(audioFormat, bytePosition / (audioFormat.getSampleSizeInBits*audioFormat.getChannels/8))
  def add(delta: Int): BytePosition = BytePosition(audioFormat, bytePosition + delta)
}
final case class SamplePosition(audioFormat: AudioFormat, samplePosition: Int) extends AudioPosition {
  Preconditions.checkArgument(samplePosition >= 0, samplePosition)
  def toTime: TimePosition =
    TimePosition(audioFormat, Duration.ofMillis((1000.0 * samplePosition.toDouble / audioFormat.getSampleRate).toInt))
  def toByte: BytePosition =
    BytePosition(audioFormat, samplePosition*audioFormat.getSampleSizeInBits*audioFormat.getChannels/8)
}
