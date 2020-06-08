package com.buransky.ostrostroj.app.audio.impl.output

import java.util.concurrent.Semaphore

import com.buransky.ostrostroj.app.audio.FrameCount
import javax.sound.sampled.SourceDataLine
import org.slf4j.LoggerFactory

import scala.annotation.tailrec

private[audio] class AsyncJavaxAudioOutput(sourceDataLine: SourceDataLine, bufferCount: Int)
  extends JavaxAudioOutput(sourceDataLine, bufferCount, new Semaphore(_)) { self =>
  import AsyncJavaxAudioOutput._

  private val thread = new Thread {
    override def run(): Unit = {
      self.write()
    }
  }
  thread.setName("audio-output")
  thread.start()
  logger.debug(s"Javax audio output thread started. [${thread.getId} - ${thread.getName}]")

  override def start(): Unit = synchronized {
    super.start()
    notifyAll()
    logger.debug("Notifying waiting threads that playback has started...")
  }

  @tailrec
  final override def write(): FrameCount = {
    val continue: Boolean = try {
      synchronized {
        if (!sourceDataLine.isRunning) {
          logger.debug("Waiting for active source data line...")
          wait()
        }
      }
      super.write()
      true
    }
    catch {
      case _: InterruptedException =>
        logger.info(s"Javax audio output thread interrupted by InterruptedException.")
        false
      case t: Throwable =>
        logger.error("Javax audio output thread failed!", t)
        throw t
    }
    if (continue && !thread.isInterrupted) {
      write()
    } else {
      logger.info(s"Javax audio output thread stopped. [$continue, ${thread.isInterrupted}]")
      FrameCount(0)
    }
  }

  override def close(): Unit = {
    thread.interrupt()
    super.close()
    logger.info("Javax audio output closed.")
  }
}

private object AsyncJavaxAudioOutput {
  private val logger = LoggerFactory.getLogger(classOf[AsyncJavaxAudioOutput])
}