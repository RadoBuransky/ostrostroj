package com.buransky.ostrostroj.app.player.looper

import java.nio.ByteBuffer

import com.buransky.ostrostroj.app.common.OstrostrojException
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
    val dst = new Array[Byte](1)

    // Execute
    val ex = intercept[OstrostrojException] {
      loopLooper.read(dst, 0)
    }

    // Assert
    assert(ex.getMessage.startsWith("Position outside of the loop!"))
  }

  it should "read the default level, single loop pass" in {
    // Prepare
    val loop = Loop(0, 1, Nil)
    val level0 = Array.fill[Byte](2)(1)
    val loopLooper = new LoopLooper(loop, audioFormat, Map(0 -> level0))
    val dst = new Array[Byte](2)

    // Execute
    val result = loopLooper.read(dst, 0)

    // Assert
    assert(result.bytesRead == 2)
    assert(result.masterSkip == 2)
    assert(dst.forall(_ == 1))
  }

  it should "read the default level, two loops" in {
    // Prepare
    val loop = Loop(0, 1, Nil)
    val level0 = Array.fill[Byte](2)(1)
    val loopLooper = new LoopLooper(loop, audioFormat, Map(0 -> level0))
    val dst = new Array[Byte](4)

    // Execute
    val result = loopLooper.read(dst, 0)

    // Assert
    assert(result.bytesRead == 4)
    assert(result.masterSkip == 2)
    assert(dst.forall(_ == 1))
  }

  it should "switch to the next level in the second pass" in {
    // Prepare
    val loop = Loop(0, 2, Nil)
    val level0 = Array.fill[Byte](4)(1)
    val level1 = Array.fill[Byte](4)(2)
    val loopLooper = new LoopLooper(loop, audioFormat, Map(0 -> level0, 1 -> level1))
    val dst = new Array[Byte](2)

    // 1. We're on the default level 0
    val r1 = loopLooper.read(dst, 0)
    assert(r1.bytesRead == 2)
    assert(r1.masterSkip == 4)
    assert(dst.forall(_ == 1))

    // 2. Switch to the next level in the middle of the first loop and check that we're still on the first level
    loopLooper.harder()
    val r2 = loopLooper.read(dst, 4)
    assert(r2.bytesRead == 2)
    assert(r2.masterSkip == 0)
    assert(dst.forall(_ == 1))

    // 3. Read more and see if we're on the second level already
    val r3 = loopLooper.read(dst, 4)
    assert(r3.bytesRead == 2)
    assert(r3.masterSkip == 0)
    assert(dst.forall(_ == 2))
  }

  it should "mix two levels" in {
    // Prepare
    val loop = Loop(0, 1, Nil)
    val level0 = Array.fill[Byte](2)(10)
    val level2 = Array.fill[Byte](2)(20)
    val loopLooper = new LoopLooper(loop, audioFormat, Map(0 -> level0, 2 -> level2))
    val dst = new Array[Byte](2)

    loopLooper.harder()
    val result = loopLooper.read(dst, 0)
    assert(result.bytesRead == 2)
    assert(result.masterSkip == 2)
    assert(dst(0) == 75)
    assert(dst(1) == 21)
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

    loopLooper.harder()
    val result = loopLooper.read(dst.array(), 0)
    assert(result.bytesRead == bufferSize)
    assert(result.masterSkip == bufferSize)
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
    val dst1 = new Array[Byte](2)

    // 1. We're looping
    for (i <- 0 to 2) {
      val r1 = loopLooper.read(dst1, 0)
      assert(r1.bytesRead == 2)
    }

    // 2. Drain
    loopLooper.startDraining()
    val r3 = loopLooper.read(dst1, 0)
    assert(r3.bytesRead == 2)

    // 3. Try to read more
    val dst2 = new Array[Byte](4)
    val r4 = loopLooper.read(dst2, 0)
    assert(r4.bytesRead == -1)
  }
}
