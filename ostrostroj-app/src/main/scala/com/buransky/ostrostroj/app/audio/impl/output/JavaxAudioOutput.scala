package com.buransky.ostrostroj.app.audio.impl.output

import java.util.concurrent.Semaphore

import com.buransky.ostrostroj.app.audio._
import javax.sound.sampled.SourceDataLine
import org.slf4j.{Logger, LoggerFactory}

import scala.collection.mutable

private[audio] class JavaxAudioOutput(sourceDataLine: SourceDataLine) extends AudioOutput {
  import JavaxAudioOutput._

  private val filledBuffers: mutable.Queue[AudioBuffer] = mutable.Queue.empty
  private val emptyBuffers: mutable.Queue[AudioBuffer] = ???
  private val filledSemaphore: Semaphore = new Semaphore(0)
  private val emptySemaphore: Semaphore = new Semaphore(prebuffers)

  override def write(): SampleCount = {
    logger.trace("Acquiring semaphore for filled buffers...")
    filledSemaphore.acquire()
    val buffer = synchronized {
      filledBuffers.dequeue()
    }
    logger.trace("Filled buffer dequeued.")
    val bytesWritten = sourceDataLine.write(buffer.byteArray, buffer.bytePosition, buffer.byteSize)
    logger.trace(s"Data written to source data line. [$bytesWritten]")

    // TODO: Add unwritten data back to the queue when stop/pause?

    synchronized {
      enqueueEmpty(buffer)
    }

    SampleCount(???) // TODO: Use bytes written
  }

  override def close(): Unit = synchronized {
    sourceDataLine.close()
  }

  override def queueFull(buffer: AudioBuffer): Unit = synchronized {
    queueFull(buffer)
    filledSemaphore.release()
    logger.trace("Full buffer enqueued.")
  }

  override def dequeued: Semaphore = emptySemaphore

  override def dequeueEmpty(): Option[AudioBuffer] = synchronized {
    if (emptyBuffers.isEmpty)
      None
    else
      Some(emptyBuffers.dequeue())
  }


  private def enqueueEmpty(buffer: AudioBuffer): Unit = {
    buffer.clear()
    emptyBuffers.enqueue(buffer)
    emptySemaphore.release()
  }
}

private object JavaxAudioOutput {
  val logger: Logger = LoggerFactory.getLogger(classOf[JavaxAudioOutput])
  val prebuffers: Int = 10
}