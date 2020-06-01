package com.buransky.ostrostroj.app.audio.impl

import com.buransky.ostrostroj.app.audio.{AudioOutput, PlaylistInput}
import org.slf4j.LoggerFactory

import scala.annotation.tailrec

private[audio] class AsyncAudioProvider(playlistInput: PlaylistInput,
                                        audioOutput: AudioOutput) extends AutoCloseable {
  self =>
  import AsyncAudioProvider._

  private val thread = new Thread {
    override def run(): Unit = {
      self.run()
    }
  }

  @tailrec
  final def run(): Unit = {
    val continue: Boolean = try {
      audioOutput.dequeued.acquire()
      audioOutput.dequeueEmpty() match {
        case Some(emptyBuffer) =>
          val fullBuffer = playlistInput.read(emptyBuffer)
          if (fullBuffer.size.value > 0) {
            audioOutput.queueFull(fullBuffer)
          } else {
            if (fullBuffer.endOfStream) {
              logger.info(s"End of stream. Stopping audio provider.")
            } else {
              logger.warn("No data but also not end of stream?")
            }
          }
          !fullBuffer.endOfStream
        case None =>
          logger.warn(s"No buffer?")
          true
      }
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
    }
  }

  override def close(): Unit = {
    thread.interrupt()
    logger.info("Async audio provider closed.")
  }
}

private object AsyncAudioProvider {
  private val logger = LoggerFactory.getLogger(classOf[AsyncAudioProvider])
}
