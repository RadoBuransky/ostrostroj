package com.buransky.ostrostroj.app.audio.impl

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
  private val emptyBuffers: mutable.Queue[Array[Byte]] =
    mutable.Queue.fill(prebuffers)(new Array[Byte](sourceDataLine.getBufferSize))
  private val filledSemaphore: Semaphore = new Semaphore(0)
  private val emptySemaphore: Semaphore = new Semaphore(prebuffers)

  def run(): Unit = {
    logger.trace("Acquiring semaphore for filled buffers...")
    filledSemaphore.acquire()
    val buffer = synchronized {
      filledBuffers.dequeue()
    }
    logger.trace("Filled buffer dequeued.")
    val bytesWritten = sourceDataLine.write(buffer.byteArray, buffer.bytePosition,
      buffer.byteLimit - buffer.bytePosition)
    logger.trace(s"Data written to source data line. [$bytesWritten]")
    synchronized {
      enqueueEmpty(buffer)
    }
  }

  override def close(): Unit = synchronized {
    sourceDataLine.close()
    mixer.close()
  }

  override def queueFull(buffer: AudioBuffer): Unit = synchronized {
    queueFull(buffer)
    filledSemaphore.release()
    logger.trace("Full buffer enqueued.")
  }

  override def dequeued: Semaphore = emptySemaphore

  override def dequeueEmpty(): Option[Array[Byte]] = synchronized {
    if (emptyBuffers.isEmpty)
      None
    else
      Some(emptyBuffers.dequeue())
  }

  private def enqueueEmpty(buffer: AudioBuffer): Unit = {
    buffer.clear()
    emptyBuffers.enqueue(buffer.byteArray)
    emptySemaphore.release()
  }
}

private object JavaSoundOutput {
  val logger: Logger = LoggerFactory.getLogger(classOf[SyncJavaSoundOutput])
  val prebuffers: Int = 5
}