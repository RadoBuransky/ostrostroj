package com.buransky.ostrostroj.app.audio.impl.output

import com.buransky.ostrostroj.app.audio.SampleCount
import javax.sound.sampled.{LineEvent, LineListener, SourceDataLine}
import org.slf4j.LoggerFactory

import scala.annotation.tailrec

private[audio] class AsyncJavaxAudioOutput(sourceDataLine: SourceDataLine) extends JavaxAudioOutput(sourceDataLine)
  with LineListener { self =>
  import AsyncJavaxAudioOutput._

  sourceDataLine.addLineListener(this)

  private val thread = new Thread {
    override def run(): Unit = {
      self.write()
    }
  }
  thread.setName("audio-output")
  logger.debug(s"Javax audio output thread started. [${thread.getId} - ${thread.getName}]")

  @tailrec
  final override def write(): SampleCount = {
    try {
      synchronized {
        if (!sourceDataLine.isActive) {
          logger.debug("Waiting for active source data line...")
          wait()
        }
      }
      super.write()
    }
    catch {
      case _: InterruptedException =>
        logger.info(s"Javax audio output thread interrupted by InterruptedException.")
      case t: Throwable =>
        logger.error("Javax audio output thread failed!", t)
        throw t
    }
    if (!thread.isInterrupted) {
      write()
    } else {
      logger.debug(s"Javax audio output thread stopped because it was interrupted.")
      SampleCount(0)
    }
  }

  override def close(): Unit = {
    thread.interrupt()
    super.close()
    logger.info("Javax audio output closed.")
  }

  override def update(event: LineEvent): Unit = synchronized {
    event match {
      case LineEvent.Type.START =>
        logger.debug("Notifying waiting threads that playback has started...")
        notifyAll()
      case _ =>
    }
  }
}

private object AsyncJavaxAudioOutput {
  private val logger = LoggerFactory.getLogger(classOf[AsyncJavaxAudioOutput])
}