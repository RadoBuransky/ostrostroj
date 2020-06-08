package com.buransky.ostrostroj.app.audio.impl.provider

import com.buransky.ostrostroj.app.audio.{AudioBuffer, AudioInput, AudioOutput, AudioProvider}
import org.slf4j.LoggerFactory

private[audio] class AudioProviderImpl extends AudioProvider {
  import AudioProviderImpl._

  def waitReadAndQueue(audioInput: AudioInput, audioOutput: AudioOutput): Boolean = {
    val emptyBuffer = audioOutput.dequeueEmpty()
    readAndQueue(emptyBuffer, audioInput, audioOutput)
  }

  private def readAndQueue(emptyBuffer: AudioBuffer, audioInput: AudioInput, audioOutput: AudioOutput): Boolean = {
    val fullBuffer: AudioBuffer = audioInput.read(emptyBuffer)
    logger.trace(s"Data read. [${fullBuffer.size}]")
    val endOfStream = fullBuffer.endOfStream
    val dataSize = fullBuffer.size.value
    audioOutput.queueFull(fullBuffer)
    if (endOfStream) {
      logger.info(s"End of playlist.")
    } else {
      if (dataSize == 0) {
        logger.warn("No data but also not end of stream?")
      }
    }
    !endOfStream
  }

  override def close(): Unit = {}
}

private object AudioProviderImpl {
  private val logger = LoggerFactory.getLogger(classOf[AudioProviderImpl])
}
