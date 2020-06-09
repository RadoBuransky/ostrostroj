package com.buransky.ostrostroj.app.audio.impl.input

import java.io.File

import com.buransky.ostrostroj.app.audio.{AudioBuffer, AudioMixer, AudioMixerChannel, FrameCount}
import com.buransky.ostrostroj.app.show.{Loop, Track}
import org.junit.runner.RunWith
import org.mockito.Mockito._
import org.mockito.{ArgumentCaptor, Matchers}
import org.scalatest.flatspec.AnyFlatSpec
import org.scalatestplus.junit.JUnitRunner
import org.scalatestplus.mockito.MockitoSugar

@RunWith(classOf[JUnitRunner])
class LoopInputImplSpec extends AnyFlatSpec with MockitoSugar {
  behavior of "constructor"

  it should "fail if starting position is before loop start" in {
    // Prepare
    val path = new File("").toPath
    val audioMixer = mock[AudioMixer]
    val loop = Loop(10, 11, List(mock[Track]))

    // Execute
    val ex = intercept[IllegalArgumentException] {
      new LoopInputImpl(loop, Nil, FrameCount(9), audioMixer)
    }

    // Assert
    assert(ex.getMessage == "Position outside of loop! [9, 10]")
  }

  it should "fail if starting position is after loop end" in {
    // Prepare
    val path = new File("").toPath
    val audioMixer = mock[AudioMixer]
    val loop = Loop(10, 11, List(mock[Track]))

    // Execute
    val ex = intercept[IllegalArgumentException] {
      new LoopInputImpl(loop, Nil, FrameCount(11), audioMixer)
    }

    // Assert
    assert(ex.getMessage == "Position outside of loop! [11, 11]")
  }

  it should "fail if not all tracks are loaded" in {
    // Prepare
    val path = new File("").toPath
    val track1 = Track(-100, -1, path)
    val track2 = Track(-1, -1, path)
    val loop = Loop(0, 1, List(track1, track2))
    val audioMixer = mock[AudioMixer]

    // Execute
    val ex = intercept[IllegalArgumentException] {
      new LoopInputImpl(loop, List(LoadedTrack(track1, mock[AudioBuffer])), FrameCount(0), audioMixer)
    }

    // Assert
    assert(ex.getMessage == "Not all tracks loaded!")
  }

  it should "set default level to 0 and find min and max levels" in {
    // Prepare
    val path = new File("").toPath
    val track1 = Track(-100, -1, path)
    val track2 = Track(-1, -1, path)
    val track3 = Track(3, 4, path)
    val audioMixer = mock[AudioMixer]
    val loop = Loop(0, 1, List(track1, track2, track3))

    // Execute
    val loopInput = new LoopInputImpl(loop, List(LoadedTrack(track1, mock[AudioBuffer]),
      LoadedTrack(track2, mock[AudioBuffer]), LoadedTrack(track3, mock[AudioBuffer])), FrameCount(0), audioMixer)

    // Assert & verify
    assert(loopInput.status.level == 0)
    assert(loopInput.status.minLevel == -100)
    assert(loopInput.status.maxLevel == 4)
  }

  behavior of "harder"

  it should "increase level by one" in {
    // Prepare
    val path = new File("").toPath
    val track1 = Track(0, 1, path)
    val audioMixer = mock[AudioMixer]
    val loop = Loop(0, 1, List(track1))
    val loopInput = new LoopInputImpl(loop, List(LoadedTrack(track1, mock[AudioBuffer])), FrameCount(0), audioMixer)

    // Execute
    val result = loopInput.harder()

    // Assert & verify
    assert(result == 1)
  }

  it should "not increase level higher than max" in {
    // Prepare
    val path = new File("").toPath
    val track1 = Track(0, 0, path)
    val audioMixer = mock[AudioMixer]
    val loop = Loop(0, 1, List(track1))
    val loopInput = new LoopInputImpl(loop, List(LoadedTrack(track1, mock[AudioBuffer])), FrameCount(0), audioMixer)

    // Execute
    val result = loopInput.harder()

    // Assert & verify
    assert(result == 0)
  }

  behavior of "softer"

  it should "decrease level by one" in {
    // Prepare
    val path = new File("").toPath
    val track1 = Track(-1, 0, path)
    val audioMixer = mock[AudioMixer]
    val loop = Loop(0, 1, List(track1))
    val loopInput = new LoopInputImpl(loop, List(LoadedTrack(track1, mock[AudioBuffer])), FrameCount(0), audioMixer)

    // Execute
    val result = loopInput.softer()

    // Assert & verify
    assert(result == -1)
  }

  it should "not decrease level lower than min" in {
    // Prepare
    val path = new File("").toPath
    val track1 = Track(0, 0, path)
    val audioMixer = mock[AudioMixer]
    val loop = Loop(0, 1, List(track1))
    val loopInput = new LoopInputImpl(loop, List(LoadedTrack(track1, mock[AudioBuffer])), FrameCount(0), audioMixer)

    // Execute
    val result = loopInput.softer()

    // Assert & verify
    assert(result == 0)
  }

  behavior of "read"

  it should "filter tracks with zero level before mixing" in {
    // Prepare
    val path = new File("").toPath
    val track1 = Track(0, 1, path)
    val byteBuffer1 = new Array[Byte](4)
    val audioBuffer1 = AudioBuffer(byteBuffer1, 4, 2, FrameCount(0), FrameCount(0), false)
    val track2 = Track(2, 3, path)
    val byteBuffer2 = new Array[Byte](4)
    val audioBuffer2 = AudioBuffer(byteBuffer2, 4, 2, FrameCount(0), FrameCount(0), false)
    val track3 = Track(-1, 1, path)
    val byteBuffer3 = new Array[Byte](4)
    val audioBuffer3 = AudioBuffer(byteBuffer3, 4, 2, FrameCount(0), FrameCount(0), false)
    val audioMixer = mock[AudioMixer]
    val loop = Loop(0, 1, List(track1, track2, track3))
    val loopInput = new LoopInputImpl(loop, List(LoadedTrack(track1, audioBuffer1), LoadedTrack(track2, audioBuffer2),
      LoadedTrack(track3, audioBuffer3)), FrameCount(0), audioMixer)
    val emptyBuffer = mock[AudioBuffer]
    when(emptyBuffer.capacity).thenReturn(FrameCount(1))
    val audioMixerChannelsCaptor = ArgumentCaptor.forClass(classOf[Iterable[AudioMixerChannel]])
    val mixedBuffer = mock[AudioBuffer]
    when(mixedBuffer.size).thenReturn(FrameCount(1))
    when(audioMixer.mix(audioMixerChannelsCaptor.capture(), Matchers.eq(emptyBuffer))).thenReturn(mixedBuffer)

    // Execute
    val result = loopInput.read(emptyBuffer)

    // Assert & verify
    assert(result == mixedBuffer)
    val audioMixerChannels = audioMixerChannelsCaptor.getValue.toList
    assert(audioMixerChannels.size == 2)
    assert(audioMixerChannels(0).audioBuffer.byteArray == byteBuffer1)
    assert(audioMixerChannels(1).audioBuffer.byteArray == byteBuffer3)
  }

  it should "determine mixing levels for all tracks" in {
    // Prepare
    val path = new File("").toPath
    val track1 = Track(0, 1, path)
    val byteBuffer1 = new Array[Byte](4)
    val audioBuffer1 = AudioBuffer(byteBuffer1, 4, 2, FrameCount(0), FrameCount(0), false)
    val track2 = Track(1, 3, path)
    val byteBuffer2 = new Array[Byte](4)
    val audioBuffer2 = AudioBuffer(byteBuffer2, 4, 2, FrameCount(0), FrameCount(0), false)
    val track3 = Track(-1, 1, path)
    val byteBuffer3 = new Array[Byte](4)
    val audioBuffer3 = AudioBuffer(byteBuffer3, 4, 2, FrameCount(0), FrameCount(0), false)
    val audioMixer = mock[AudioMixer]
    val loop = Loop(0, 1, List(track1, track2, track3))
    val loopInput = new LoopInputImpl(loop, List(LoadedTrack(track1, audioBuffer1), LoadedTrack(track2, audioBuffer2),
      LoadedTrack(track3, audioBuffer3)), FrameCount(0), audioMixer)
    val emptyBuffer = mock[AudioBuffer]
    when(emptyBuffer.capacity).thenReturn(FrameCount(1))
    val audioMixerChannelsCaptor = ArgumentCaptor.forClass(classOf[Iterable[AudioMixerChannel]])
    val mixedBuffer = mock[AudioBuffer]
    when(mixedBuffer.size).thenReturn(FrameCount(1))
    when(audioMixer.mix(audioMixerChannelsCaptor.capture(), Matchers.eq(emptyBuffer))).thenReturn(mixedBuffer)

    // Execute
    val result = loopInput.read(emptyBuffer)

    // Assert & verify
    assert(result == mixedBuffer)
    val audioMixerChannels = audioMixerChannelsCaptor.getValue.toList
    assert(audioMixerChannels.size == 2)
    assert(audioMixerChannels(0).audioBuffer.byteArray == byteBuffer1)
    assert(audioMixerChannels(0).level == 1.0)
    assert(audioMixerChannels(1).audioBuffer.byteArray == byteBuffer3)
    assert(audioMixerChannels(1).level == 1.0)
  }

  it should "limit channel view only to match input buffer size" in {
    // Prepare
    val path = new File("").toPath
    val track1 = Track(0, 1, path)
    val byteBuffer1 = new Array[Byte](256)
    val audioBuffer1 = AudioBuffer(byteBuffer1, 4, 2, FrameCount(0), FrameCount(0), false)
    val audioMixer = mock[AudioMixer]
    val loop = Loop(10, 20, List(track1))
    val loopInput = new LoopInputImpl(loop, List(LoadedTrack(track1, audioBuffer1)), FrameCount(15), audioMixer)
    val emptyBuffer = mock[AudioBuffer]
    when(emptyBuffer.capacity).thenReturn(FrameCount(1))
    val audioMixerChannelsCaptor = ArgumentCaptor.forClass(classOf[Iterable[AudioMixerChannel]])
    val mixedBuffer = mock[AudioBuffer]
    when(mixedBuffer.size).thenReturn(FrameCount(1))
    when(audioMixer.mix(audioMixerChannelsCaptor.capture(), Matchers.eq(emptyBuffer))).thenReturn(mixedBuffer)

    // Execute
    val result = loopInput.read(emptyBuffer)

    // Assert & verify
    assert(result == mixedBuffer)
    val audioMixerChannels = audioMixerChannelsCaptor.getValue.toList
    assert(audioMixerChannels.size == 1)
    assert(audioMixerChannels(0).audioBuffer.position == FrameCount(5))
    assert(audioMixerChannels(0).audioBuffer.limit == FrameCount(6))
  }

  it should "reset position to the start of the loop if not draining" in {
    // Prepare
    val path = new File("").toPath
    val track1 = Track(0, 1, path)
    val byteBuffer1 = new Array[Byte](256)
    val audioBuffer1 = AudioBuffer(byteBuffer1, 4, 2, FrameCount(0), FrameCount(10), false)
    val audioMixer = mock[AudioMixer]
    val loop = Loop(10, 20, List(track1))
    val loopInput = new LoopInputImpl(loop, List(LoadedTrack(track1, audioBuffer1)), FrameCount(19), audioMixer)
    val emptyBuffer = mock[AudioBuffer]
    when(emptyBuffer.capacity).thenReturn(FrameCount(1))
    val audioMixerChannelsCaptor = ArgumentCaptor.forClass(classOf[Iterable[AudioMixerChannel]])
    val mixedBuffer = AudioBuffer(byteBuffer1, 4, 2, FrameCount(0), FrameCount(1), false)
    when(audioMixer.mix(audioMixerChannelsCaptor.capture(), Matchers.eq(emptyBuffer))).thenReturn(mixedBuffer)

    // Execute
    for (i <- 0 to 50) {
      loopInput.read(emptyBuffer)
      val status = loopInput.status

      // Assert & verify
      assert(status.position == FrameCount(((9 + i) % 10) + 11))
    }
  }

  it should "return end of stream after draining is done" in {
    // Prepare
    val path = new File("").toPath
    val track1 = Track(0, 1, path)
    val byteBuffer1 = new Array[Byte](256)
    val audioBuffer1 = AudioBuffer(byteBuffer1, 4, 2, FrameCount(0), FrameCount(10), false)
    val audioMixer = mock[AudioMixer]
    val loop = Loop(10, 20, List(track1))
    val loopInput = new LoopInputImpl(loop, List(LoadedTrack(track1, audioBuffer1)), FrameCount(15), audioMixer)
    val emptyBuffer = AudioBuffer(byteBuffer1, 4, 2, FrameCount(0), FrameCount(10), false)
    val audioMixerChannelsCaptor = ArgumentCaptor.forClass(classOf[Iterable[AudioMixerChannel]])
    val mixedBuffer = AudioBuffer(byteBuffer1, 4, 2, FrameCount(0), FrameCount(5), false)
    when(audioMixer.mix(audioMixerChannelsCaptor.capture(), Matchers.eq(emptyBuffer))).thenReturn(mixedBuffer)

    // Execute
    loopInput.startDraining()
    val result1 = loopInput.read(emptyBuffer)
    val status1 = loopInput.status
    val result2 = loopInput.read(emptyBuffer)
    val status2 = loopInput.status

    // Assert & verify
    assert(result1 == mixedBuffer)
    assert(status1.position == FrameCount(20))
    assert(!result1.endOfStream)
    assert(result2.endOfStream)
    assert(status2.position == FrameCount(20))
  }
}
