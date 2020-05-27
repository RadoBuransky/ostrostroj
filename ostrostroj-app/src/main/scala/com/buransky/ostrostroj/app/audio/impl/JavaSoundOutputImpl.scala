package com.buransky.ostrostroj.app.audio.impl

import java.util.concurrent.Semaphore

import com.buransky.ostrostroj.app.audio.{AudioBuffer, ByteCount, JavaSoundOutput}
import javax.sound.sampled.{AudioFormat, Mixer, SourceDataLine}
import org.slf4j.LoggerFactory

import scala.annotation.tailrec
import scala.collection.mutable

private[audio] class JavaSoundOutputImpl(audioFormat: AudioFormat) extends JavaSoundOutput {
  import JavaSoundOutputImpl._

  logger.info("Java sound output execution context created.")

  private val mixer: Mixer = ??? // TODO: ...
  private val sourceDataLine: SourceDataLine = ??? // TODO: ...

  private val filledBuffers: mutable.Queue[AudioBuffer] = mutable.Queue.empty
  private val emptyBuffers: mutable.Queue[AudioBuffer] = {
    mutable.Queue.fill(prebuffers)(AudioBuffer(audioFormat, ByteCount(sourceDataLine.getBufferSize)))
  }
  private val filledSemaphore: Semaphore = new Semaphore(0)
  private val emptySemaphore: Semaphore = new Semaphore(prebuffers)

  private val thread = new Thread {
    @tailrec
    override def run(): Unit = {
      try {
        logger.trace("Waiting for semaphore.")
        filledSemaphore.acquire()
        val buffer = synchronized {
          filledBuffers.dequeue()
        }
        logger.trace("Filled buffer dequeued.")
        sourceDataLine.write(buffer.raw.array(), buffer.raw.position(), buffer.raw.limit() - buffer.raw.position())

        // TODO: Update current playback position retrieved from AudioBuffer

        logger.trace("Data written to source data line.")
        synchronized {
          enqueueEmpty(buffer)
        }
      }
      catch {
        case _: InterruptedException =>
          logger.info(s"Java sound output thread interrupted by InterruptedException.")
        case t: Throwable =>
          logger.error("Java sound output thread failed!", t)
          throw t
      }
      if (!isInterrupted) {
        run()
      } else {
        logger.info(s"Java sound output thread stopped because it was interrupted.")
      }
    }
  }

  override def close(): Unit = synchronized {
    thread.interrupt()
    sourceDataLine.close()
    mixer.close()
    logger.info("Java sound output closed.")
  }

  override def enequeueFull(buffer: AudioBuffer): Unit = synchronized {
    enequeueFull(buffer)
    filledSemaphore.release()
    logger.trace("Full buffer enqueued.")
  }

  override def dequeueEmpty(): Option[AudioBuffer] = synchronized {
    if (emptyBuffers.isEmpty) {
      None
    } else {
      emptySemaphore.acquire()
      Some(emptyBuffers.dequeue())
    }
  }

  override def emptyAvailable: Semaphore = emptySemaphore

  override def flush(): Unit = synchronized {
    while (filledBuffers.nonEmpty) {
      enqueueEmpty(filledBuffers.dequeue())
    }
  }

  private def enqueueEmpty(buffer: AudioBuffer): Unit = {
    buffer.clear()
    emptyBuffers.enqueue(buffer)
    emptySemaphore.release()
  }
}

private object JavaSoundOutputImpl {
  private val logger = LoggerFactory.getLogger(classOf[JavaSoundOutputImpl])
  private val prebuffers = 5
}