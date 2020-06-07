package com.buransky.ostrostroj.app.audio.impl.input

import java.io.File

import com.buransky.ostrostroj.app.audio.{AudioBuffer, FrameCount, LoopInput, LoopStatus}
import com.buransky.ostrostroj.app.show.{Loop, Song}
import javax.sound.sampled.{AudioFormat, AudioInputStream}
import org.junit.runner.RunWith
import org.mockito.Matchers
import org.mockito.Mockito._
import org.mockito.Matchers._
import org.scalatest.flatspec.AnyFlatSpec
import org.scalatestplus.junit.JUnitRunner
import org.scalatestplus.mockito.MockitoSugar

@RunWith(classOf[JUnitRunner])
class SongInputImplSpec extends AnyFlatSpec with MockitoSugar {
  private val audioFormat = new AudioFormat(44100, 16, 2, true, false)

  behavior of "startLooping"

  it should "do nothing if there's no loop at the current position" in {
    // Prepare
    val song = Song("", new File("").toPath, Nil)
    val masterTrackInputStream = mock[AudioInputStream]
    def loopInputFactory(loop: Loop, position: FrameCount): LoopInput = fail()
    val songInput = new SongInputImpl(song, masterTrackInputStream, loopInputFactory)

    // Execute
    songInput.startLooping()

    // Verify
    verifyZeroInteractions(masterTrackInputStream)
  }

  it should "create loop input and skip master track" in {
    // Prepare
    val loop = Loop(0, 7, Nil)
    val song = Song("", new File("").toPath, List(loop))
    val masterTrackInputStream = mock[AudioInputStream]
    val loopInputFactory = mock[(Loop, FrameCount) => LoopInput]
    val loopInput = mock[LoopInput]
    val songInput = new SongInputImpl(song, masterTrackInputStream, loopInputFactory)
    when(masterTrackInputStream.getFormat).thenReturn(audioFormat)
    when(loopInputFactory(loop, FrameCount(0))).thenReturn(loopInput)

    // Execute
    songInput.startLooping()

    // Verify
    verify(loopInputFactory).apply(loop, FrameCount(0))
    verify(masterTrackInputStream).getFormat
    verify(masterTrackInputStream).skip(28)
    verifyNoMoreInteractions(masterTrackInputStream)
    verifyNoMoreInteractions(loopInputFactory)
    verifyNoMoreInteractions(loopInput)
  }

  it should "stop draining if we are already looping" in {
    // Prepare
    val loop = Loop(0, 7, Nil)
    val song = Song("", new File("").toPath, List(loop))
    val masterTrackInputStream = mock[AudioInputStream]
    val loopInputFactory = mock[(Loop, FrameCount) => LoopInput]
    val loopInput = mock[LoopInput]
    val songInput = new SongInputImpl(song, masterTrackInputStream, loopInputFactory)
    when(masterTrackInputStream.getFormat).thenReturn(audioFormat)
    when(loopInputFactory(loop, FrameCount(0))).thenReturn(loopInput)

    // Execute
    songInput.startLooping()
    songInput.startLooping()

    // Verify
    verify(loopInput).stopDraining()
    verify(loopInputFactory).apply(loop, FrameCount(0))
    verify(masterTrackInputStream).getFormat
    verify(masterTrackInputStream).skip(28)
    verifyNoMoreInteractions(masterTrackInputStream)
    verifyNoMoreInteractions(loopInputFactory)
    verifyNoMoreInteractions(loopInput)
  }

  behavior of "stopLooping"

  it should "do nothing if we're not looping" in {
    // Prepare
    val song = Song("", new File("").toPath, Nil)
    val masterTrackInputStream = mock[AudioInputStream]
    val loopInputFactory = mock[(Loop, FrameCount) => LoopInput]
    val songInput = new SongInputImpl(song, masterTrackInputStream, loopInputFactory)

    // Execute
    songInput.stopLooping()

    // Verify
    verifyZeroInteractions(loopInputFactory)
    verifyZeroInteractions(masterTrackInputStream)
  }

  it should "start draining if we're looping" in {
    // Prepare
    val loop = Loop(0, 7, Nil)
    val song = Song("", new File("").toPath, List(loop))
    val masterTrackInputStream = mock[AudioInputStream]
    val loopInputFactory = mock[(Loop, FrameCount) => LoopInput]
    val loopInput = mock[LoopInput]
    val songInput = new SongInputImpl(song, masterTrackInputStream, loopInputFactory)
    when(masterTrackInputStream.getFormat).thenReturn(audioFormat)
    when(loopInputFactory(loop, FrameCount(0))).thenReturn(loopInput)

    // Execute
    songInput.startLooping()
    songInput.stopLooping()

    // Verify
    verify(loopInput).startDraining()
    verify(loopInputFactory).apply(loop, FrameCount(0))
    verify(masterTrackInputStream).getFormat
    verify(masterTrackInputStream).skip(28)
    verifyNoMoreInteractions(masterTrackInputStream)
    verifyNoMoreInteractions(loopInputFactory)
    verifyNoMoreInteractions(loopInput)
  }

  behavior of "toggleLooping"

  it should "start or stop looping" in {
    // Prepare
    val loop = Loop(0, 7, Nil)
    val song = Song("", new File("").toPath, List(loop))
    val masterTrackInputStream = mock[AudioInputStream]
    val loopInputFactory = mock[(Loop, FrameCount) => LoopInput]
    val loopInput = mock[LoopInput]
    val songInput = new SongInputImpl(song, masterTrackInputStream, loopInputFactory)
    when(masterTrackInputStream.getFormat).thenReturn(audioFormat)
    when(loopInputFactory(loop, FrameCount(0))).thenReturn(loopInput)

    // Step 1
    songInput.toggleLooping()
    verify(loopInputFactory).apply(loop, FrameCount(0))
    verify(masterTrackInputStream).getFormat
    verify(masterTrackInputStream).skip(28)
    verifyNoMoreInteractions(masterTrackInputStream)
    verifyNoMoreInteractions(loopInputFactory)
    verifyNoMoreInteractions(loopInput)

    // Step 2
    songInput.toggleLooping()
    verify(loopInput).toggleDraining()
    verifyNoMoreInteractions(loopInput)
  }

  behavior of "loopInput"

  it should "return loop input" in {
    // Prepare
    val loop = Loop(0, 7, Nil)
    val song = Song("", new File("").toPath, List(loop))
    val masterTrackInputStream = mock[AudioInputStream]
    val loopInputFactory = mock[(Loop, FrameCount) => LoopInput]
    val loopInput = mock[LoopInput]
    val songInput = new SongInputImpl(song, masterTrackInputStream, loopInputFactory)
    when(masterTrackInputStream.getFormat).thenReturn(audioFormat)
    when(loopInputFactory(loop, FrameCount(0))).thenReturn(loopInput)

    // Execute
    songInput.startLooping()
    val result = songInput.loopInput

    // Assert & verify
    assert(result.contains(loopInput))
    verify(loopInputFactory).apply(loop, FrameCount(0))
    verify(masterTrackInputStream).getFormat
    verify(masterTrackInputStream).skip(28)
    verifyNoMoreInteractions(masterTrackInputStream)
    verifyNoMoreInteractions(loopInputFactory)
    verifyNoMoreInteractions(loopInput)
  }

  behavior of "read"

  it should "read from master track when we're not looping" in {
    // Prepare
    val song = Song("", new File("").toPath, Nil)
    val masterTrackInputStream = mock[AudioInputStream]
    def loopInputFactory(loop: Loop, position: FrameCount): LoopInput = fail()
    val songInput = new SongInputImpl(song, masterTrackInputStream, loopInputFactory)
    val emptyBuffer = mock[AudioBuffer]
    val fullBuffer = mock[AudioBuffer]
    val byteBuffer = new Array[Byte](1)
    when(emptyBuffer.byteArray).thenReturn(byteBuffer)
    when(masterTrackInputStream.read(byteBuffer)).thenReturn(16)
    when(masterTrackInputStream.getFormat).thenReturn(audioFormat)
    when(emptyBuffer.copy(any(), any(), any(), Matchers.eq(FrameCount(0)), Matchers.eq(FrameCount(4)), any()))
      .thenReturn(fullBuffer)

    // Execute
    val result = songInput.read(emptyBuffer)

    // Assert & verify
    assert(result == fullBuffer)
    verify(masterTrackInputStream).getFormat
    verify(masterTrackInputStream).read(byteBuffer)
    verify(emptyBuffer).capacity
    verify(emptyBuffer).byteArray
    verify(emptyBuffer).copy(any(), any(), any(), Matchers.eq(FrameCount(0)), Matchers.eq(FrameCount(4)), any())
    verifyNoMoreInteractions(masterTrackInputStream)
    verifyNoMoreInteractions(fullBuffer)
  }

  it should "read from master track and handle end of stream when we're not looping" in {
    // Prepare
    val song = Song("", new File("").toPath, Nil)
    val masterTrackInputStream = mock[AudioInputStream]
    def loopInputFactory(loop: Loop, position: FrameCount): LoopInput = fail()
    val songInput = new SongInputImpl(song, masterTrackInputStream, loopInputFactory)
    val emptyBuffer = mock[AudioBuffer]
    val fullBuffer = mock[AudioBuffer]
    val byteBuffer = new Array[Byte](1)
    when(emptyBuffer.byteArray).thenReturn(byteBuffer)
    when(masterTrackInputStream.read(byteBuffer)).thenReturn(-1)
    when(masterTrackInputStream.getFormat).thenReturn(audioFormat)
    when(emptyBuffer.copy(any(), any(), any(), Matchers.eq(FrameCount(0)), Matchers.eq(FrameCount(0)),
      Matchers.eq(true))).thenReturn(fullBuffer)

    // Execute
    val result = songInput.read(emptyBuffer)

    // Assert & verify
    assert(result == fullBuffer)
    verify(masterTrackInputStream).read(byteBuffer)
    verify(emptyBuffer).capacity
    verify(emptyBuffer).byteArray
    verify(emptyBuffer).copy(any(), any(), any(), Matchers.eq(FrameCount(0)), Matchers.eq(FrameCount(0)),
      Matchers.eq(true))
    verifyNoMoreInteractions(masterTrackInputStream)
    verifyNoMoreInteractions(fullBuffer)
  }

  it should "read only from loop input when looping" in {
    // Prepare
    val loop = Loop(0, 7, Nil)
    val song = Song("", new File("").toPath, List(loop))
    val masterTrackInputStream = mock[AudioInputStream]
    val loopInputFactory = mock[(Loop, FrameCount) => LoopInput]
    val loopInput = mock[LoopInput]
    val songInput = new SongInputImpl(song, masterTrackInputStream, loopInputFactory)
    val emptyBuffer = mock[AudioBuffer]
    val fullBuffer = mock[AudioBuffer]
    when(masterTrackInputStream.getFormat).thenReturn(audioFormat)
    when(loopInputFactory(loop, FrameCount(0))).thenReturn(loopInput)
    when(loopInput.read(emptyBuffer)).thenReturn(fullBuffer)
    when(fullBuffer.endOfStream).thenReturn(false)

    // Execute
    songInput.startLooping()
    val result = songInput.read(emptyBuffer)

    // Verify
    assert(result == fullBuffer)
    verify(loopInput).read(emptyBuffer)
    verify(loopInputFactory).apply(loop, FrameCount(0))
    verify(masterTrackInputStream).getFormat
    verify(masterTrackInputStream).skip(28)
    verify(fullBuffer).endOfStream
    verify(emptyBuffer).capacity
    verifyNoMoreInteractions(masterTrackInputStream)
    verifyNoMoreInteractions(loopInputFactory)
    verifyNoMoreInteractions(loopInput)
    verifyNoMoreInteractions(fullBuffer)
    verifyNoMoreInteractions(emptyBuffer)
  }

  it should "finish reading from loop input" in {
    // Prepare
    val loop = Loop(0, 7, Nil)
    val song = Song("", new File("").toPath, List(loop))
    val masterTrackInputStream = mock[AudioInputStream]
    val loopInputFactory = mock[(Loop, FrameCount) => LoopInput]
    val loopInput = mock[LoopInput]
    val songInput = new SongInputImpl(song, masterTrackInputStream, loopInputFactory)
    val emptyBuffer = mock[AudioBuffer]
    val fullBuffer = mock[AudioBuffer]
    val fullBufferNotEnd = mock[AudioBuffer]
    when(masterTrackInputStream.getFormat).thenReturn(audioFormat)
    when(loopInputFactory(loop, FrameCount(0))).thenReturn(loopInput)
    when(loopInput.read(emptyBuffer)).thenReturn(fullBuffer)
    when(fullBuffer.endOfStream).thenReturn(true)
    when(fullBuffer.copy(any(), any(), any(), any(), any(), Matchers.eq(false))).thenReturn(fullBufferNotEnd)

    // Execute
    songInput.startLooping()
    val result = songInput.read(emptyBuffer)

    // Verify
    assert(result == fullBufferNotEnd)
    verify(loopInput).read(emptyBuffer)
    verify(loopInput).close()
    verify(loopInputFactory).apply(loop, FrameCount(0))
    verify(masterTrackInputStream).getFormat
    verify(masterTrackInputStream).skip(28)
    verify(fullBuffer).endOfStream
    verify(emptyBuffer).capacity
    verifyNoMoreInteractions(masterTrackInputStream)
    verifyNoMoreInteractions(loopInputFactory)
    verifyNoMoreInteractions(loopInput)
    verifyNoMoreInteractions(emptyBuffer)
  }

  behavior of "close"

  it should "close loop input if exists" in {
    // Prepare
    val loop = Loop(0, 7, Nil)
    val song = Song("", new File("").toPath, List(loop))
    val masterTrackInputStream = mock[AudioInputStream]
    val loopInputFactory = mock[(Loop, FrameCount) => LoopInput]
    val loopInput = mock[LoopInput]
    val songInput = new SongInputImpl(song, masterTrackInputStream, loopInputFactory)
    when(masterTrackInputStream.getFormat).thenReturn(audioFormat)
    when(loopInputFactory(loop, FrameCount(0))).thenReturn(loopInput)

    // Execute
    songInput.startLooping()
    songInput.close()

    // Verify
    verify(loopInput).close()
    verify(loopInputFactory).apply(loop, FrameCount(0))
    verify(masterTrackInputStream).getFormat
    verify(masterTrackInputStream).skip(28)
    verifyNoMoreInteractions(masterTrackInputStream)
    verifyNoMoreInteractions(loopInputFactory)
    verifyNoMoreInteractions(loopInput)
  }

  behavior of "status"

  it should "get status from loop input if exists" in {
    // Prepare
    val loop = Loop(0, 7, Nil)
    val song = Song("", new File("").toPath, List(loop))
    val masterTrackInputStream = mock[AudioInputStream]
    val loopInputFactory = mock[(Loop, FrameCount) => LoopInput]
    val loopInput = mock[LoopInput]
    val loopStatus = mock[LoopStatus]
    val songInput = new SongInputImpl(song, masterTrackInputStream, loopInputFactory)
    when(masterTrackInputStream.getFormat).thenReturn(audioFormat)
    when(loopInputFactory(loop, FrameCount(0))).thenReturn(loopInput)
    when(loopInput.status).thenReturn(loopStatus)

    // Execute
    songInput.startLooping()
    val result = songInput.status

    // Verify
    assert(result.song == song)
    assert(result.loopStatus.contains(loopStatus))
    verify(loopInput, times(2)).status
    verify(loopStatus).position
    verify(loopInputFactory).apply(loop, FrameCount(0))
    verify(masterTrackInputStream).getFormat
    verify(masterTrackInputStream).skip(28)
    verifyNoMoreInteractions(masterTrackInputStream)
    verifyNoMoreInteractions(loopInputFactory)
    verifyNoMoreInteractions(loopInput)
    verifyNoMoreInteractions(loopStatus)
  }

  behavior of "status.position"

  it should "equal to master track position when not looping" in {
    // Prepare
    val song = Song("", new File("").toPath, Nil)
    val masterTrackInputStream = mock[AudioInputStream]
    def loopInputFactory(loop: Loop, position: FrameCount): LoopInput = fail()
    val songInput = new SongInputImpl(song, masterTrackInputStream, loopInputFactory)
    val emptyBuffer = mock[AudioBuffer]
    val fullBuffer = mock[AudioBuffer]
    val byteBuffer = new Array[Byte](1)
    when(emptyBuffer.byteArray).thenReturn(byteBuffer)
    when(masterTrackInputStream.read(byteBuffer)).thenReturn(2048)
    when(masterTrackInputStream.getFormat).thenReturn(audioFormat)
    when(emptyBuffer.copy(any(), any(), any(), Matchers.eq(FrameCount(0)), Matchers.eq(FrameCount(512)), any()))
      .thenReturn(fullBuffer)

    // Execute
    songInput.read(emptyBuffer)
    val status = songInput.status

    // Assert & verify
    assert(status.position == FrameCount(512))
    verify(masterTrackInputStream).getFormat
    verify(masterTrackInputStream).read(byteBuffer)
    verify(emptyBuffer).capacity
    verify(emptyBuffer).byteArray
    verify(emptyBuffer).copy(any(), any(), any(), Matchers.eq(FrameCount(0)), Matchers.eq(FrameCount(512)), any())
    verifyNoMoreInteractions(masterTrackInputStream)
    verifyNoMoreInteractions(fullBuffer)
  }

  it should "equal to loop input position when looping" in {
    // Prepare
    val loop = Loop(0, 7, Nil)
    val song = Song("", new File("").toPath, List(loop))
    val masterTrackInputStream = mock[AudioInputStream]
    val loopInputFactory = mock[(Loop, FrameCount) => LoopInput]
    val loopInput = mock[LoopInput]
    val songInput = new SongInputImpl(song, masterTrackInputStream, loopInputFactory)
    val emptyBuffer = mock[AudioBuffer]
    val fullBuffer = mock[AudioBuffer]
    val loopStatus = mock[LoopStatus]
    when(masterTrackInputStream.getFormat).thenReturn(audioFormat)
    when(loopInputFactory(loop, FrameCount(0))).thenReturn(loopInput)
    when(loopInput.status).thenReturn(loopStatus)
    when(loopStatus.position).thenReturn(FrameCount(123))

    // Execute
    songInput.startLooping()
    val status = songInput.status

    // Verify
    assert(status.position == FrameCount(123))
    verify(loopInputFactory).apply(loop, FrameCount(0))
    verify(masterTrackInputStream).getFormat
    verify(masterTrackInputStream).skip(28)
    verify(loopInput, times(2)).status
    verify(loopStatus).position
    verifyNoMoreInteractions(masterTrackInputStream)
    verifyNoMoreInteractions(loopInputFactory)
    verifyNoMoreInteractions(loopInput)
    verifyNoMoreInteractions(fullBuffer)
    verifyNoMoreInteractions(emptyBuffer)
    verifyNoMoreInteractions(loopStatus)
  }

  it should "switch from loop position to master track after draining is done" in {
    // Prepare
    val loop = Loop(0, 7, Nil)
    val song = Song("", new File("").toPath, List(loop))
    val masterTrackInputStream = mock[AudioInputStream]
    val loopInputFactory = mock[(Loop, FrameCount) => LoopInput]
    val loopInput = mock[LoopInput]
    val songInput = new SongInputImpl(song, masterTrackInputStream, loopInputFactory)
    val emptyBuffer = mock[AudioBuffer]
    val fullBuffer = mock[AudioBuffer]
    val loopStatus = mock[LoopStatus]
    when(masterTrackInputStream.getFormat).thenReturn(audioFormat)
    when(masterTrackInputStream.skip(28)).thenReturn(28)
    when(loopInputFactory(loop, FrameCount(0))).thenReturn(loopInput)
    when(loopInput.status).thenReturn(loopStatus)
    when(loopStatus.position).thenReturn(FrameCount(123))
    when(loopInput.read(emptyBuffer)).thenReturn(fullBuffer)
    when(fullBuffer.endOfStream).thenReturn(true)

    // Execute
    songInput.startLooping()
    songInput.stopLooping()
    songInput.read(emptyBuffer)
    val status = songInput.status

    // Verify
    assert(status.loopStatus.isEmpty)
    assert(status.position == FrameCount(7))
    verify(loopInputFactory).apply(loop, FrameCount(0))
    verify(masterTrackInputStream).getFormat
    verify(masterTrackInputStream).skip(28)
    verify(loopInput).startDraining()
    verify(loopInput).read(emptyBuffer)
    verify(loopInput).close()
    verify(fullBuffer).endOfStream
    verify(emptyBuffer).capacity
    verifyNoMoreInteractions(masterTrackInputStream)
    verifyNoMoreInteractions(loopInputFactory)
    verifyNoMoreInteractions(loopInput)
    verifyNoMoreInteractions(emptyBuffer)
    verifyNoMoreInteractions(loopStatus)
  }

  it should "still use loop input position while draining" in {
    // Prepare
    val loop = Loop(0, 7, Nil)
    val song = Song("", new File("").toPath, List(loop))
    val masterTrackInputStream = mock[AudioInputStream]
    val loopInputFactory = mock[(Loop, FrameCount) => LoopInput]
    val loopInput = mock[LoopInput]
    val songInput = new SongInputImpl(song, masterTrackInputStream, loopInputFactory)
    val emptyBuffer = mock[AudioBuffer]
    val fullBuffer = mock[AudioBuffer]
    val loopStatus = mock[LoopStatus]
    when(masterTrackInputStream.getFormat).thenReturn(audioFormat)
    when(masterTrackInputStream.skip(28)).thenReturn(28)
    when(loopInputFactory(loop, FrameCount(0))).thenReturn(loopInput)
    when(loopInput.status).thenReturn(loopStatus)
    when(loopStatus.position).thenReturn(FrameCount(123))
    when(loopInput.read(emptyBuffer)).thenReturn(fullBuffer)
    when(fullBuffer.endOfStream).thenReturn(false)

    // Execute
    songInput.startLooping()
    songInput.stopLooping()
    songInput.read(emptyBuffer)
    val status = songInput.status

    // Verify
    assert(status.loopStatus.contains(loopStatus))
    assert(status.position == FrameCount(123))
    verify(loopInputFactory).apply(loop, FrameCount(0))
    verify(masterTrackInputStream).getFormat
    verify(masterTrackInputStream).skip(28)
    verify(loopInput).startDraining()
    verify(loopInput).read(emptyBuffer)
    verify(fullBuffer).endOfStream
    verify(emptyBuffer).capacity
    verify(loopInput, times(2)).status
    verify(loopStatus).position
    verifyNoMoreInteractions(masterTrackInputStream)
    verifyNoMoreInteractions(loopInputFactory)
    verifyNoMoreInteractions(loopInput)
    verifyNoMoreInteractions(emptyBuffer)
    verifyNoMoreInteractions(loopStatus)
  }
}
