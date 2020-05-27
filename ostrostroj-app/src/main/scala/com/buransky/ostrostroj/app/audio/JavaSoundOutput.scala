package com.buransky.ostrostroj.app.audio

import java.nio.ByteBuffer
import java.util.concurrent.Semaphore

import com.buransky.ostrostroj.app.audio.impl.JavaSoundOutputImpl
import javax.sound.sampled.AudioFormat

private[audio] trait JavaSoundOutput extends AutoCloseable {
  def enequeueFull(buffer: AudioBuffer): Unit
  def dequeueEmpty(): Option[AudioBuffer]
  def emptyAvailable: Semaphore
  def flush(): Unit
}

private[audio] object JavaSoundOutput {
  def apply(audioFormat: AudioFormat): JavaSoundOutput = new JavaSoundOutputImpl(audioFormat)
}