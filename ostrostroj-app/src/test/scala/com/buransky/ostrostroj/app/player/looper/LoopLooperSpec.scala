package com.buransky.ostrostroj.app.player.looper

import java.nio.ByteBuffer

import com.buransky.ostrostroj.app.common.OstrostrojException
import com.buransky.ostrostroj.app.player.BytePosition
import com.buransky.ostrostroj.app.show.Loop
import javax.sound.sampled.AudioFormat
import org.junit.runner.RunWith
import org.scalatest.flatspec.AnyFlatSpec
import org.scalatestplus.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
class LoopLooperSpec extends AnyFlatSpec {
  private val audioFormat = new AudioFormat(44100, 16, 1, true, false)

  behavior of "read"

  it should "fail if position is outside of the loop" in {
    // Prepare
    val loop = Loop(1, 2, Nil)
    val loopLooper = new LoopLooper(loop, audioFormat, Map.empty)
    val dst = ByteBuffer.allocate(1)
    dst.limit(0)

    // Execute
    val ex = intercept[OstrostrojException] {
      loopLooper.fill(dst, BytePosition(audioFormat, 0))
    }

    // Assert
    assert(ex.getMessage.startsWith("Position outside of the loop!"))
  }

  it should "read the default level, single loop pass" in {
    // Prepare
    val loop = Loop(0, 1, Nil)
    val level0 = Array.fill[Byte](2)(1)
    val loopLooper = new LoopLooper(loop, audioFormat, Map(0 -> level0))
    val dst = ByteBuffer.allocate(2)
    dst.limit(0)

    // Execute
    val masterSkip = loopLooper.fill(dst, BytePosition(audioFormat, 0))

    // Assert
    assert(dst.limit() == 2)
    assert(masterSkip.bytePosition == 2)
    for (i <- dst.position() until dst.limit())
      assert(dst.get(i) == 1)
  }

  it should "read the default level, two loops" in {
    // Prepare
    val loop = Loop(0, 1, Nil)
    val level0 = Array.fill[Byte](2)(1)
    val loopLooper = new LoopLooper(loop, audioFormat, Map(0 -> level0))
    val dst = ByteBuffer.allocate(4)
    dst.limit(0)

    // Execute
    val masterSkip = loopLooper.fill(dst, BytePosition(audioFormat, 0))

    // Assert
    assert(dst.limit() == 4)
    assert(masterSkip.bytePosition == 2)
    for (i <- dst.position() until dst.limit())
      assert(dst.get(i) == 1)
  }

  it should "switch to the next level in the second pass" in {
    // Prepare
    val loop = Loop(0, 2, Nil)
    val level0 = Array.fill[Byte](4)(1)
    val level1 = Array.fill[Byte](4)(2)
    val loopLooper = new LoopLooper(loop, audioFormat, Map(0 -> level0, 1 -> level1))
    val dst = ByteBuffer.allocate(2)
    dst.limit(0)

    // 1. We're on the default level 0
    val masterSkip1 = loopLooper.fill(dst, BytePosition(audioFormat, 0))
    assert(dst.limit() == 2)
    assert(masterSkip1.bytePosition == 4)
    for (i <- dst.position() until dst.limit())
      assert(dst.get(i) == 1)

    // 2. Switch to the next level in the middle of the first loop and check that we're still on the first level
    loopLooper.harder()
    dst.position(0)
    dst.limit(0)
    val masterSkip2 = loopLooper.fill(dst, BytePosition(audioFormat, 4))
    assert(dst.limit() == 2)
    assert(masterSkip2.bytePosition == 0)
    for (i <- dst.position() until dst.limit())
      assert(dst.get(i) == 1)

    // 3. Read more and see if we're on the second level already
    dst.position(0)
    dst.limit(0)
    val masterSkip3 = loopLooper.fill(dst, BytePosition(audioFormat, 4))
    assert(dst.limit() == 2)
    assert(masterSkip3.bytePosition == 0)
    for (i <- dst.position() until dst.limit())
      assert(dst.get(i) == 2)
  }

  it should "mix two levels" in {
    // Prepare
    val loop = Loop(0, 1, Nil)
    val level0 = Array.fill[Byte](2)(10)
    val level2 = Array.fill[Byte](2)(20)
    val loopLooper = new LoopLooper(loop, audioFormat, Map(0 -> level0, 2 -> level2))
    val dst = ByteBuffer.allocate(2)
    dst.limit(0)

    loopLooper.harder()
    val masterSkip = loopLooper.fill(dst, BytePosition(audioFormat, 0))
    assert(dst.limit() == 2)
    assert(masterSkip.bytePosition == 2)
    assert(dst.get(0) == 75)
    assert(dst.get(1) == 21)
  }

  it should "crossfade from one level to another" in {
    // Prepare
    val bufferSize = BufferMixer.xfadingBufferLength(audioFormat)*2
    val loop = Loop(0, bufferSize/(audioFormat.getChannels*audioFormat.getSampleSizeInBits/8), Nil)
    val level0 = ByteBuffer.allocate(bufferSize)
    val level1 = ByteBuffer.allocate(bufferSize)
    for (i <- 0 until bufferSize/2) {
      level0.putShort(0x0001)
      level1.putShort(0x0002)
    }
    val loopLooper = new LoopLooper(loop, audioFormat, Map(0 -> level0.array(), 1 -> level1.array()))
    val dst = ByteBuffer.allocate(bufferSize)
    dst.limit(0)

    loopLooper.harder()
    val masterSkip = loopLooper.fill(dst, BytePosition(audioFormat, 0))
    assert(dst.limit() == bufferSize)
    assert(masterSkip.bytePosition == bufferSize)
    assert(dst.getShort == 0x0001)
    assert(dst.getShort(bufferSize/16) == 0xA401.toShort)
    assert(dst.getShort(bufferSize/8) == 0xDD01.toShort)
    assert(dst.getShort(bufferSize/4) == 0x1F02)
    assert(dst.getShort(bufferSize/2) == 0x0002)
    assert(dst.getShort(3*bufferSize/4) == 0x0002)
    assert(dst.getShort(bufferSize - 2) == 0x0002)
  }

  behavior of "draining"

  it should "return data until end of loop and then finish" in {
    // Prepare
    val loop = Loop(0, 2, Nil)
    val level0 = Array.fill[Byte](4)(1)
    val loopLooper = new LoopLooper(loop, audioFormat, Map(0 -> level0))
    val dst1 = ByteBuffer.allocate(2)

    // 1. We're looping
    for (i <- 0 to 2) {
      dst1.position(0)
      dst1.limit(0)
      loopLooper.fill(dst1, BytePosition(audioFormat, 0))
      assert(dst1.limit() == 2)
    }

    // 2. Drain
    loopLooper.startDraining()
    dst1.position(0)
    dst1.limit(0)
    loopLooper.fill(dst1, BytePosition(audioFormat, 0))
    assert(dst1.limit() == 2)

    // 3. Try to read more
    val dst2 = ByteBuffer.allocate(4)
    dst2.limit(0)
    loopLooper.fill(dst2, BytePosition(audioFormat, 0))
    assert(dst2.limit() == 0)
  }
}
