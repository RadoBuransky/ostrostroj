package com.buransky.ostrostroj.app.audio.impl

import java.nio.ByteBuffer
import java.util.concurrent.Semaphore

import com.buransky.ostrostroj.app.audio._
import com.buransky.ostrostroj.app.audio.impl.JavaSoundOutput.logger
import javax.sound.sampled.{AudioFormat, Mixer, SourceDataLine}
import org.slf4j.{Logger, LoggerFactory}

import scala.annotation.tailrec
import scala.collection.mutable

private[audio] class AsyncJavaSoundOutput(audioFormat: AudioFormat) extends SyncJavaSoundOutput(audioFormat) {
  self =>

  private val thread = new Thread {
    override def run(): Unit = {
      self.run()
    }
  }

  @tailrec
  final override def run(): Unit = {
    try {
      super.run()
    }
    catch {
      case _: InterruptedException =>
        logger.info(s"Java sound output thread interrupted by InterruptedException.")
      case t: Throwable =>
        logger.error("Java sound output thread failed!", t)
        throw t
    }
    if (!thread.isInterrupted) {
      run()
    } else {
      logger.info(s"Java sound output thread stopped because it was interrupted.")
    }
  }

  override def close(): Unit = {
    thread.interrupt()
    super.close()
    logger.info("Async Java sound output closed.")
  }
}

private[audio] class SyncJavaSoundOutput(audioFormat: AudioFormat) extends AudioOutput {
  import JavaSoundOutput._

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


  override def run(): Unit = {
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

  override def close(): Unit = synchronized {
    sourceDataLine.close()
    mixer.close()
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

private object JavaSoundOutput {
  val logger: Logger = LoggerFactory.getLogger(classOf[SyncJavaSoundOutput])
  val prebuffers: Int = 5
}