package com.buransky.ostrostroj.app.audio.impl.output

import java.util.concurrent.Semaphore

import com.buransky.ostrostroj.app.audio.AudioBuffer
import javax.sound.sampled.{AudioFormat, FloatControl, SourceDataLine}
import org.junit.runner.RunWith
import org.mockito.Mockito._
import org.scalatest.flatspec.AnyFlatSpec
import org.scalatestplus.junit.JUnitRunner
import org.scalatestplus.mockito.MockitoSugar

import scala.annotation.tailrec

@RunWith(classOf[JUnitRunner])
class JavaxAudioOutputSpec extends AnyFlatSpec with MockitoSugar {
  private val audioFormat = new AudioFormat(44100, 16, 2, true, false)

  behavior of "constructor"

  it should "create empty buffers and retrieve gain control" in new StandardFixture {
    // Execute
    val javaxAudioOutput = new JavaxAudioOutput(sourceDataLine, 3, semaphoreFactory)

    // Assert & verify
    assert(dequeueAll(javaxAudioOutput, Nil).length == 3)
    verify(sourceDataLine).getControl(FloatControl.Type.MASTER_GAIN)
    verify(sourceDataLine, times(3)).getBufferSize
    verify(sourceDataLine, atLeastOnce()).getFormat
    verifyNoMoreInteractions(sourceDataLine)
  }

  behavior of "close"

  it should "close source data line" in new StandardFixture {
    // Prepare
    val javaxAudioOutput = new JavaxAudioOutput(sourceDataLine, 1, semaphoreFactory)

    // Execute
    javaxAudioOutput.close()

    // Verify
    verify(sourceDataLine).close()
  }

  behavior of "write"

  it should "process queued full buffer" in new StandardFixture {
    // Prepare
    val javaxAudioOutput = new JavaxAudioOutput(sourceDataLine, 1, semaphoreFactory)
    val audioBuffer = javaxAudioOutput.dequeueEmpty()
    assert(javaxAudioOutput.tryDequeueEmpty().isEmpty)
    javaxAudioOutput.queueFull(audioBuffer)
    val frameCount = 42
    when(sourceDataLine.write(audioBuffer.byteArray, audioBuffer.bytePosition, audioBuffer.byteSize))
      .thenReturn(audioFormat.getFrameSize*frameCount)

    // Execute
    val result = javaxAudioOutput.write()

    // Assert
    assert(javaxAudioOutput.dequeueEmpty() == audioBuffer)
    assert(result.value == frameCount)
    verify(sourceDataLine).write(audioBuffer.byteArray, audioBuffer.bytePosition, audioBuffer.byteSize)
    verify(emptySemaphore, times(2)).acquire()
    verify(emptySemaphore).release()
    verify(filledSemaphore).release()
    verify(filledSemaphore).acquire()
    verifyCommon()
    verifyNoMoreInteractions(sourceDataLine)
    verifyNoMoreInteractions(filledSemaphore)
    verifyNoMoreInteractions(emptySemaphore)
  }

  behavior of "volumeUp"

  it should "increase current volume by a step" in new StandardFixture {
    // Prepare
    val javaxAudioOutput = new JavaxAudioOutput(sourceDataLine, 1, semaphoreFactory)
    when(gainControl.getValue).thenReturn(5)
    when(gainControl.getMaximum).thenReturn(10)
    when(gainControl.getMinimum).thenReturn(0)

    // Execute
    val result = javaxAudioOutput.volumeUp()

    // Assert & verify
    assert(result == 0.6)
    verify(gainControl, times(2)).getMaximum
    verify(gainControl, times(3)).getMinimum
    verify(gainControl).getValue
    verify(gainControl).setValue(6)
    verifyNoMoreInteractions(gainControl)
  }

  it should "not set value higher than max" in new StandardFixture {
    // Prepare
    val javaxAudioOutput = new JavaxAudioOutput(sourceDataLine, 1, semaphoreFactory)
    when(gainControl.getValue).thenReturn(9)
    when(gainControl.getMaximum).thenReturn(10)
    when(gainControl.getMinimum).thenReturn(0)

    // Execute
    val result = javaxAudioOutput.volumeUp()

    // Assert & verify
    assert(result == 1.0)
    verify(gainControl, times(3)).getMaximum
    verify(gainControl, times(2)).getMinimum
    verify(gainControl).getValue
    verify(gainControl).setValue(10)
    verifyNoMoreInteractions(gainControl)
  }

  behavior of "volumeDown"

  it should "decrease current volume by a step" in new StandardFixture {
    // Prepare
    val javaxAudioOutput = new JavaxAudioOutput(sourceDataLine, 1, semaphoreFactory)
    when(gainControl.getValue).thenReturn(2)
    when(gainControl.getMaximum).thenReturn(5)
    when(gainControl.getMinimum).thenReturn(0)

    // Execute
    val result = javaxAudioOutput.volumeDown()

    // Assert & verify
    assert(result == 0.0)
    verify(gainControl, times(2)).getMaximum
    verify(gainControl, times(4)).getMinimum
    verify(gainControl).getValue
    verify(gainControl).setValue(0)
    verifyNoMoreInteractions(gainControl)
  }

  behavior of "volume"

  it should "return ratio of current volume and allowed range" in new StandardFixture {
    // Prepare
    val javaxAudioOutput = new JavaxAudioOutput(sourceDataLine, 1, semaphoreFactory)
    when(gainControl.getValue).thenReturn(3)
    when(gainControl.getMaximum).thenReturn(6)
    when(gainControl.getMinimum).thenReturn(-6)

    // Execute
    val result = javaxAudioOutput.volume

    // Assert & verify
    assert(result == 0.75)
    verify(gainControl).getMaximum
    verify(gainControl, times(2)).getMinimum
    verify(gainControl).getValue
    verifyNoMoreInteractions(gainControl)
  }

  private class StandardFixture {
    // Prepare
    val sourceDataLine: SourceDataLine = mock[SourceDataLine]
    val gainControl: FloatControl = mock[FloatControl]
    val filledSemaphore: Semaphore = mock[Semaphore]
    val emptySemaphore: Semaphore = mock[Semaphore]
    protected def semaphoreFactory(count: Int): Semaphore = count match {
      case 0 => filledSemaphore
      case _ => emptySemaphore
    }
    when(sourceDataLine.getFormat).thenReturn(audioFormat)
    when(sourceDataLine.getBufferSize).thenReturn(audioFormat.getFrameSize)
    when(sourceDataLine.getControl(FloatControl.Type.MASTER_GAIN)).thenReturn(gainControl)

    def verifyCommon(): Unit = {
      verify(sourceDataLine).getControl(FloatControl.Type.MASTER_GAIN)
      verify(sourceDataLine, atLeastOnce()).getBufferSize
      verify(sourceDataLine, atLeastOnce()).getFormat
    }
  }

  @tailrec
  private def dequeueAll(javaxAudioOutput: JavaxAudioOutput, acc: List[AudioBuffer]): List[AudioBuffer] = {
    javaxAudioOutput.tryDequeueEmpty() match {
      case Some(audioBuffer) => dequeueAll(javaxAudioOutput, audioBuffer :: acc)
      case None => acc
    }
  }
}
