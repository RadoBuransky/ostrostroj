package com.buransky.ostrostroj.app.audio.impl.provider

import com.buransky.ostrostroj.app.audio.{AudioInput, AudioOutput}
import org.slf4j.LoggerFactory

import scala.annotation.tailrec

private[audio] class AsyncAudioProvider(audioInput: AudioInput, audioOutput: AudioOutput) extends AudioProviderImpl {
  self =>
  import AsyncAudioProvider._

  private val thread = new Thread {
    override def run(): Unit = {
      self.run()
    }
  }
  thread.setName("audio-provider")
  thread.start()
  logger.debug(s"Async audio provider thread started. [${thread.getId} - ${thread.getName}]")

  @tailrec
  final def run(): Unit = {
    val continue: Boolean = try {
      waitReadAndQueue(audioInput, audioOutput)
    }
    catch {
      case _: InterruptedException =>
        logger.info(s"Async audio provider thread interrupted by InterruptedException.")
        false
      case t: Throwable =>
        logger.error("Async audio provider thread failed!", t)
        throw t
    }
    if (continue && !thread.isInterrupted) {
      run()
    } else {
      logger.debug(s"Async audio provider thread stopped. [$continue, ${thread.isInterrupted}]")
    }
  }

  override def close(): Unit = {
    thread.interrupt()
    super.close()
    logger.info("Async audio provider closed.")
  }
}

private object AsyncAudioProvider {
  private val logger = LoggerFactory.getLogger(classOf[AudioProviderImpl])
}
