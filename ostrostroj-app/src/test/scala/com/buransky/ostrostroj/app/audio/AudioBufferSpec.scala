package com.buransky.ostrostroj.app.audio

import java.io.File
import java.nio.file.Path

import javax.sound.sampled.spi.AudioFileReader
import javax.sound.sampled.{AudioFormat, AudioInputStream}
import org.junit.runner.RunWith
import org.mockito.Matchers._
import org.mockito.Mockito._
import org.scalatest.flatspec.AnyFlatSpec
import org.scalatestplus.junit.JUnitRunner
import org.scalatestplus.mockito.MockitoSugar

@RunWith(classOf[JUnitRunner])
class AudioBufferSpec extends AnyFlatSpec with MockitoSugar {
  private val audioBuffer = new AudioBuffer(new Array[Byte](12), 4, FrameCount(1), FrameCount(2), false)

  behavior of "constructor"

  it should "fail if byteArray is empty" in {
    val ex = intercept[IllegalArgumentException] {
      new AudioBuffer(Array.empty[Byte], 4, FrameCount(0), FrameCount(0), false)
    }
    assert(ex.getMessage == "byteArray is empty.")
  }

  it should "fail if frameSize is weird" in {
    val ex = intercept[IllegalArgumentException] {
      new AudioBuffer(new Array[Byte](4), 3, FrameCount(0), FrameCount(0), false)
    }
    assert(ex.getMessage == "Unsupported frameSize. [3]")
  }

  it should "fail if position is negative" in {
    val ex = intercept[IllegalArgumentException] {
      new AudioBuffer(new Array[Byte](4), 4, FrameCount(-1), FrameCount(0), false)
    }
    assert(ex.getMessage == "Negative position. [-1]")
  }

  it should "fail if limit is negative" in {
    val ex = intercept[IllegalArgumentException] {
      new AudioBuffer(new Array[Byte](4), 4, FrameCount(0), FrameCount(-1), false)
    }
    assert(ex.getMessage == "Negative limit. [-1]")
  }

  it should "fail if limit is smaller than position" in {
    val ex = intercept[IllegalArgumentException] {
      new AudioBuffer(new Array[Byte](4), 4, FrameCount(2), FrameCount(1), false)
    }
    assert(ex.getMessage == "Limit smaller than position. [2, 1]")
  }

  it should "fail if position is outside of byteBuffer" in {
    val ex = intercept[IllegalArgumentException] {
      new AudioBuffer(new Array[Byte](4), 4, FrameCount(1), FrameCount(1), false)
    }
    assert(ex.getMessage == "Position outside of byteBuffer. [1, 1]")
  }

  it should "fail if limit is outside of byteBuffer" in {
    val ex = intercept[IllegalArgumentException] {
      new AudioBuffer(new Array[Byte](4), 4, FrameCount(0), FrameCount(2), false)
    }
    assert(ex.getMessage == "Limit outside of byteBuffer. [2, 1]")
  }

  it should "not fail if limit equals to capacity of byteBuffer" in {
    new AudioBuffer(new Array[Byte](4), 4, FrameCount(0), FrameCount(1), false)
  }

  behavior of "size"

  it should "compute frame size" in {
    assert(audioBuffer.size == FrameCount(1))
  }

  behavior of "capacity"

  it should "compute frame capacity" in {
    assert(audioBuffer.capacity == FrameCount(3))
  }

  behavior of "bytePosition"

  it should "compute position in number of bytes" in {
    assert(audioBuffer.bytePosition == 4)
  }

  behavior of "byteLimit"

  it should "compute limit in number of bytes" in {
    assert(audioBuffer.byteLimit == 9)
  }

  behavior of "byteSize"

  it should "compute size in number of bytes" in {
    assert(audioBuffer.byteSize == 4)
  }

  behavior of "byteCapacity"

  it should "compute capacity in number of bytes" in {
    assert(audioBuffer.byteCapacity == 12)
  }

  behavior of "apply(AudioFormat, Int)"

  it should "fail if byteSize is not aligned" in {
    val audioFormat = new AudioFormat(44100, 16, 2, true, false)
    val ex = intercept[IllegalArgumentException] {
      AudioBuffer.apply(audioFormat, 5)
    }
    assert(ex.getMessage == "Illegal byteSize. [5, 4]")
  }

  it should "allocate memory for byte buffer" in {
    // Prepare
    val audioFormat = new AudioFormat(44100, 16, 2, true, false)

    // Execute
    val result = AudioBuffer(audioFormat, 4)

    // Assert
    assert(result.byteArray.length == 4)
    assert(result.position == FrameCount(0))
    assert(result.limit == FrameCount(0))
    assert(!result.endOfStream)
  }

  behavior of "apply(Path, AudioFileReader)"

  it should "load audio file into audio buffer" in {
    // Prepare
    val path = mock[Path]
    val file = mock[File]
    val audioFileReader = mock[AudioFileReader]
    val audioInputStream = mock[AudioInputStream]
    val audioFormat = new AudioFormat(44100, 16, 2, true, false)
    when(path.toFile).thenReturn(file)
    when(audioFileReader.getAudioInputStream(file)).thenReturn(audioInputStream)
    when(audioInputStream.available()).thenReturn(8)
    when(audioInputStream.getFormat).thenReturn(audioFormat)
    when(audioInputStream.read(any())).thenReturn(8)

    // Execute
    val result = AudioBuffer(path, audioFileReader)

    // Assert & verify
    assert(result.byteArray.length == 8)
    assert(result.position == FrameCount(0))
    assert(result.limit == FrameCount(2))
    assert(result.endOfStream)
    verify(audioInputStream).close()
    verify(audioInputStream).read(any())
    verify(audioInputStream).getFormat
    verify(audioInputStream).available()
    verify(audioFileReader).getAudioInputStream(file)
    verify(path).toFile
  }
}
