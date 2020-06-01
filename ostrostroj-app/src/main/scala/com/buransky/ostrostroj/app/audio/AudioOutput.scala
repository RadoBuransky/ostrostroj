package com.buransky.ostrostroj.app.audio

import java.util.concurrent.Semaphore

private[audio] trait AudioOutput extends AutoCloseable {
  def write(): SampleCount

  def queueFull(buffer: AudioBuffer): Unit
  def dequeueEmpty(): Option[AudioBuffer]
  def dequeued: Semaphore
}