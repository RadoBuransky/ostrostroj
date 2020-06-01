package com.buransky.ostrostroj.app.audio.impl

import com.buransky.ostrostroj.app.audio.SampleCount
import javax.sound.sampled.{LineEvent, LineListener, SourceDataLine}
import org.slf4j.LoggerFactory

import scala.annotation.tailrec

private[audio] class AsyncJavaSoundOutput(sourceDataLine: SourceDataLine) extends JavaSoundOutput(sourceDataLine)
  with LineListener { self =>
  import AsyncJavaSoundOutput._

  sourceDataLine.addLineListener(this)

  private val thread = new Thread {
    override def run(): Unit = {
      self.write()
    }
  }

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
        logger.info(s"Java sound output thread interrupted by InterruptedException.")
      case t: Throwable =>
        logger.error("Java sound output thread failed!", t)
        throw t
    }
    if (!thread.isInterrupted) {
      write()
    } else {
      logger.info(s"Java sound output thread stopped because it was interrupted.")
      SampleCount(0)
    }
  }

  override def close(): Unit = {
    thread.interrupt()
    super.close()
    logger.info("Async Java sound output closed.")
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

private object AsyncJavaSoundOutput {
  private val logger = LoggerFactory.getLogger(classOf[AsyncJavaSoundOutput])
}