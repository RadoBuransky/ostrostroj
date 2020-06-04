package com.buransky.ostrostroj.app.audio.impl.provider

import com.buransky.ostrostroj.app.audio.{AudioBuffer, AudioInput, AudioOutput, AudioProvider}
import org.slf4j.LoggerFactory

private[audio] class AudioProviderImpl extends AudioProvider {
  import AudioProviderImpl._

  def waitReadAndQueue(audioInput: AudioInput, audioOutput: AudioOutput): Boolean = {
    val emptyBuffer = audioOutput.dequeueEmpty()
    val fullBuffer = readAndQueue(emptyBuffer, audioInput, audioOutput)
    !fullBuffer.endOfStream
  }

  private def readAndQueue(emptyBuffer: AudioBuffer, audioInput: AudioInput, audioOutput: AudioOutput): AudioBuffer = {
    val fullBuffer: AudioBuffer = audioInput.read(emptyBuffer)
    if (fullBuffer.size.value > 0) {
      audioOutput.queueFull(fullBuffer)
    } else {
      if (fullBuffer.endOfStream) {
        logger.info(s"End of playlist stream - we're done. Stopping audio provider thread.")
      } else {
        logger.warn("No data but also not end of stream?")
      }
    }
    fullBuffer
  }

  override def close(): Unit = {}
}

private object AudioProviderImpl {
  private val logger = LoggerFactory.getLogger(classOf[AudioProviderImpl])
}
