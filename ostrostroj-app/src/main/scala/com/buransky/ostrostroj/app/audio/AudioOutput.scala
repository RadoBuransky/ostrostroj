package com.buransky.ostrostroj.app.audio

import java.nio.ByteBuffer
import java.util.concurrent.Semaphore

import com.buransky.ostrostroj.app.audio.impl.AsyncJavaSoundOutput
import javax.sound.sampled.AudioFormat

private[audio] trait AudioOutput extends AutoCloseable {
  def run(): Unit

  def write(buffer: AudioBuffer): Unit
  def nextEmpty(): Option[ByteBuffer]
  def emptyAvailable: Semaphore
  def flush(): Unit

  def playbackPosition: Option[PlaybackPosition]
  def bufferingPosition: Option[PlaybackPosition]
}

private[audio] object AudioOutput {
  def apply(audioFormat: AudioFormat): AudioOutput = new AsyncJavaSoundOutput(audioFormat)
}