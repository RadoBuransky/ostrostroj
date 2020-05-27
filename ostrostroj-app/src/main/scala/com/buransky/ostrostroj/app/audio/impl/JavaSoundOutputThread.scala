package com.buransky.ostrostroj.app.audio.impl

import java.nio.ByteBuffer
import java.util.concurrent.Semaphore

import com.buransky.ostrostroj.app.audio._
import javax.sound.sampled.{AudioFormat, Mixer, SourceDataLine}
import org.slf4j.LoggerFactory

import scala.annotation.tailrec
import scala.collection.mutable

private[audio] class JavaSoundOutputThread(audioFormat: AudioFormat) extends JavaSoundOutput {
  import JavaSoundOutputThread._

  logger.info("Java sound output execution context created.")

  private val mixer: Mixer = ??? // TODO: ...
  private val sourceDataLine: SourceDataLine = ??? // TODO: ...

  private val filledBuffers: mutable.Queue[AudioBuffer] = mutable.Queue.empty
  private val emptyBuffers: mutable.Queue[ByteBuffer] =
    mutable.Queue.fill(prebuffers)(ByteBuffer.allocate(sourceDataLine.getBufferSize))
  private val filledSemaphore: Semaphore = new Semaphore(0)
  private val emptySemaphore: Semaphore = new Semaphore(prebuffers)
  /**
   * End position of the last buffer written to the source data line.
   */
  private var _playbackPosition: Option[PlaybackPosition] = None
  /**
   * End position of the last buffer queued to prebuffering queue.
   */
  private var _bufferingPosition: Option[PlaybackPosition] = None

  private val thread = new Thread {
    @tailrec
    override def run(): Unit = {
      try {
        logger.trace("Acquiring semaphore for filled buffers...")
        filledSemaphore.acquire()
        val buffer = synchronized {
          filledBuffers.dequeue()
        }
        logger.trace("Filled buffer dequeued.")
        val bytesWritten = sourceDataLine.write(buffer.raw.array(), buffer.raw.position(),
          buffer.raw.limit() - buffer.raw.position())
        logger.trace(s"Data written to source data line. [$bytesWritten]")

        // TODO: bytesWritten?

        _playbackPosition = Some(buffer.endPosition)
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

  override def write(buffer: AudioBuffer): Unit = synchronized {
    write(buffer)
    filledSemaphore.release()
    logger.trace("Full buffer enqueued.")
  }

  override def nextEmpty(): Option[ByteBuffer] = synchronized {
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
    emptyBuffers.enqueue(buffer.raw)
    emptySemaphore.release()
  }

  override def playbackPosition: Option[PlaybackPosition] = _playbackPosition
  override def bufferingPosition: Option[PlaybackPosition] = _bufferingPosition
}

private object JavaSoundOutputThread {
  private val logger = LoggerFactory.getLogger(classOf[JavaSoundOutputThread])
  private val prebuffers = 5
}