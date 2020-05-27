package com.buransky.ostrostroj.app.audio

import java.nio.ByteBuffer
import java.util.concurrent.Semaphore

import com.buransky.ostrostroj.app.audio.impl.JavaSoundOutputThread
import javax.sound.sampled.AudioFormat

private[audio] trait JavaSoundOutput extends AutoCloseable {
  def write(buffer: AudioBuffer): Unit
  def nextEmpty(): Option[ByteBuffer]
  def emptyAvailable: Semaphore
  def flush(): Unit

  def playbackPosition: Option[PlaybackPosition]
  def bufferingPosition: Option[PlaybackPosition]
}

private[audio] object JavaSoundOutput {
  def apply(audioFormat: AudioFormat): JavaSoundOutput = new JavaSoundOutputThread(audioFormat)
}