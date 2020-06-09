package com.buransky.ostrostroj.app.audio.impl.provider

import com.buransky.ostrostroj.app.audio.{AudioBuffer, AudioInput, AudioOutput, FrameCount}
import org.junit.runner.RunWith
import org.scalatest.flatspec.AnyFlatSpec
import org.scalatestplus.junit.JUnitRunner
import org.scalatestplus.mockito.MockitoSugar
import org.mockito.Mockito._

@RunWith(classOf[JUnitRunner])
class AudioProviderImplSpec extends AnyFlatSpec with MockitoSugar {
  behavior of "waitReadAndQueue"

  it should "wait, read and then queue" in {
    // Prepare
    val audioProvider = new AudioProviderImpl()
    val audioInput = mock[AudioInput]
    val audioOutput = mock[AudioOutput]
    val emptyBuffer = mock[AudioBuffer]
    val fullBuffer = mock[AudioBuffer]
    when(audioOutput.dequeueEmpty()).thenReturn(emptyBuffer)
    when(audioInput.read(emptyBuffer)).thenReturn(fullBuffer)
    when(fullBuffer.size).thenReturn(FrameCount(1))
    when(fullBuffer.endOfStream).thenReturn(false)

    // Execute
    val result = audioProvider.waitReadAndQueue(audioInput, audioOutput)

    // Assert & verify
    assert(result)
    verify(fullBuffer).endOfStream
    verify(fullBuffer, times(2)).size
    verify(audioOutput).queueFull(fullBuffer)
    verify(audioInput).read(emptyBuffer)
    verify(audioOutput).dequeueEmpty()
    verifyNoMoreInteractions(audioInput)
    verifyNoMoreInteractions(audioOutput)
    verifyNoMoreInteractions(emptyBuffer)
    verifyNoMoreInteractions(fullBuffer)
  }

  it should "wait, read and queue even if no data is returned" in {
    // Prepare
    val audioProvider = new AudioProviderImpl()
    val audioInput = mock[AudioInput]
    val audioOutput = mock[AudioOutput]
    val emptyBuffer = mock[AudioBuffer]
    val fullBuffer = mock[AudioBuffer]
    when(audioOutput.dequeueEmpty()).thenReturn(emptyBuffer)
    when(audioInput.read(emptyBuffer)).thenReturn(fullBuffer)
    when(fullBuffer.size).thenReturn(FrameCount(0))
    when(fullBuffer.endOfStream).thenReturn(true)

    // Execute
    val result = audioProvider.waitReadAndQueue(audioInput, audioOutput)

    // Assert & verify
    assert(!result)
    verify(fullBuffer).endOfStream
    verify(fullBuffer, times(2)).size
    verify(audioInput).read(emptyBuffer)
    verify(audioOutput).dequeueEmpty()
    verify(audioOutput).queueFull(fullBuffer)
    verifyNoMoreInteractions(audioInput)
    verifyNoMoreInteractions(audioOutput)
    verifyNoMoreInteractions(emptyBuffer)
    verifyNoMoreInteractions(fullBuffer)
  }
}
