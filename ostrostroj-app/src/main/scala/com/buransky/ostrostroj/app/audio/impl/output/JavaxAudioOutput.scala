package com.buransky.ostrostroj.app.audio.impl.output

import java.util.concurrent.Semaphore

import com.buransky.ostrostroj.app.audio._
import com.buransky.ostrostroj.app.common.OstrostrojException
import javax.sound.sampled.{FloatControl, SourceDataLine}
import org.slf4j.LoggerFactory

import scala.collection.mutable

private[audio] class JavaxAudioOutput(sourceDataLine: SourceDataLine, bufferCount: Int) extends AudioOutput {
  import JavaxAudioOutput._

  private val filledBuffers = mutable.Queue.empty[AudioBuffer]
  private val emptyBuffers = createEmptyBuffers(bufferCount)
  private val filledSemaphore = new Semaphore(0)
  private val emptySemaphore = new Semaphore(bufferCount)
  private val gainControl = getGainControl(sourceDataLine)

  override def write(): FrameCount = {
    logger.trace("Acquiring semaphore for filled buffers...")
    filledSemaphore.acquire()
    val buffer = synchronized {
      filledBuffers.dequeue()
    }
    logger.trace("Filled buffer dequeued.")
    val bytesWritten = sourceDataLine.write(buffer.byteArray, buffer.bytePosition, buffer.byteSize)
    logger.trace(s"Data written to source data line. [$bytesWritten, ${buffer.byteSize}]")
    synchronized {
      enqueueEmpty(buffer)
      FrameCount(bytesWritten/sourceDataLine.getFormat.getFrameSize)
    }
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

  override def volumeUp(): Unit = changeVolume(1)

  override def volumeDown(): Unit = changeVolume(-1)

  private def changeVolume(stepDelta: Int): Unit =
    gainControl.setValue(((gainControl.getValue.toInt / volumeStepDb) + stepDelta)*volumeStepDb)

  private def enqueueEmpty(buffer: AudioBuffer): Unit = {
    val recycledBuffer = buffer.copy(position = FrameCount(0), limit = FrameCount(0), endOfStream = false)
    emptyBuffers.enqueue(recycledBuffer)
    emptySemaphore.release()
  }

  private def createEmptyBuffers(bufferCount: Int): mutable.Queue[AudioBuffer] =
    mutable.Queue.fill(bufferCount)(AudioBuffer(sourceDataLine.getFormat, sourceDataLine.getBufferSize))

  private def getGainControl(sourceDataLine: SourceDataLine): FloatControl = {
    sourceDataLine.getControl(FloatControl.Type.MASTER_GAIN) match {
      case fc: FloatControl => fc
      case other => throw new OstrostrojException(s"Master gain is not a FloatControl! [${other.getClass}]")
    }
  }
}

private object JavaxAudioOutput {
  private val logger = LoggerFactory.getLogger(classOf[JavaxAudioOutput])
  private val volumeStepDb = 3
}