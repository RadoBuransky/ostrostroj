package com.buransky.ostrostroj.app.audio

import java.util.concurrent.Semaphore

import com.buransky.ostrostroj.app.audio.impl.AsyncJavaSoundOutput
import javax.sound.sampled.AudioFormat

private[audio] trait AudioOutput extends AutoCloseable {
  def queueFull(buffer: AudioBuffer): Unit
  def dequeueEmpty(): Option[Array[Byte]]
  def dequeued: Semaphore
}

private[audio] object AudioOutput {
  def apply(audioFormat: AudioFormat): AudioOutput = new AsyncJavaSoundOutput(audioFormat)
}