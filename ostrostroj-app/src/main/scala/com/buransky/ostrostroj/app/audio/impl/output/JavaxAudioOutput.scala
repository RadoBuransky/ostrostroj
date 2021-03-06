package com.buransky.ostrostroj.app.audio.impl.output

import java.util.concurrent.Semaphore

import com.buransky.ostrostroj.app.audio._
import com.buransky.ostrostroj.app.common.OstrostrojException
import javax.sound.sampled.{FloatControl, SourceDataLine}
import org.slf4j.LoggerFactory

import scala.annotation.tailrec
import scala.collection.mutable

private[audio] class JavaxAudioOutput(sourceDataLine: SourceDataLine,
                                      bufferCount: Int,
                                      semaphoreFactory: (Int) => Semaphore) extends AudioOutput {
  import JavaxAudioOutput._

  private val filledBuffers = mutable.Queue.empty[AudioBuffer]
  private val emptyBuffers = createEmptyBuffers(bufferCount)
  private val filledSemaphore = semaphoreFactory(0)
  private val emptySemaphore = semaphoreFactory(bufferCount)
  private val gainControl = getGainControl(sourceDataLine)

  private var totalFramesWritten = FrameCount(0)

  override def start(): Unit = {
    sourceDataLine.start()
    logger.debug("Playback started.")
  }

  override def stop(): Unit = {
    sourceDataLine.stop()
    logger.debug("Playback stopped.")
  }

  override def write(): FrameCount = {
    logger.trace("Acquiring semaphore for filled buffers...")
    filledSemaphore.acquire()
    val buffer = synchronized {
      filledBuffers.dequeue()
    }
    logger.trace("Filled buffer dequeued.")
    val bytesWritten = sourceDataLine.write(buffer.byteArray, buffer.bytePosition, buffer.byteSize)
    if (logger.isTraceEnabled()) {
      logger.trace(s"Data written to source data line. [$bytesWritten, ${buffer.byteSize}, " +
        s"${sourceDataLine.available()}]")
    }
    synchronized {
      val framesWritten = FrameCount(bytesWritten/sourceDataLine.getFormat.getFrameSize)
      totalFramesWritten += framesWritten
      enqueueEmpty(buffer)
      framesWritten
    }
  }

  override def close(): Unit = synchronized {
    sourceDataLine.close()
  }

  override def queueFull(buffer: AudioBuffer): Unit = synchronized {
    filledBuffers.enqueue(buffer)
    filledSemaphore.release()
    logger.trace("Full buffer enqueued.")
  }

  @tailrec
  override final def dequeueEmpty(): AudioBuffer = {
    logger.trace(s"Waiting for an empty buffer...")
    emptySemaphore.acquire()
    if (logger.isTraceEnabled) {
      val bufferingStatus = 100 - (100*(emptySemaphore.availablePermits())/bufferCount)
      logger.trace(s"Empty buffer acquired. [$bufferingStatus% buffers full]")
    }
    tryDequeueEmpty() match {
      case Some(audioBuffer) => audioBuffer
      case None => dequeueEmpty()
    }
  }

  override def tryDequeueEmpty(): Option[AudioBuffer] = synchronized {
    if (emptyBuffers.isEmpty)
      None
    else
      Some(emptyBuffers.dequeue())
  }

  override def volumeUp(): Double = changeVolume(1)

  override def volumeDown(): Double = changeVolume(-1)

  override def volume: Double = volume(gainControl.getValue, gainControl.getMinimum, gainControl.getMaximum)

  override def framesBuffered: FrameCount = synchronized {
    FrameCount(totalFramesWritten.value - sourceDataLine.getFramePosition)
  }

  private def changeVolume(stepDelta: Int): Double = {
    val max = gainControl.getMaximum
    val min = gainControl.getMinimum
    val newValue = ((gainControl.getValue.toInt / volumeStepDb) + stepDelta)*volumeStepDb
    val valueToSet = if (newValue > max) {
      max
    } else {
      if (newValue < min)
        min
      else
        newValue
    }
    gainControl.setValue(valueToSet)
    val result = volume(valueToSet, min, max)
    logger.debug(s"Change volume. [${result}, $valueToSet, $min, $max]")
    result
  }

  private def volume(currentValue: Double, min: Double, max: Double): Double =
    Math.abs((currentValue - min) / (max - min))

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