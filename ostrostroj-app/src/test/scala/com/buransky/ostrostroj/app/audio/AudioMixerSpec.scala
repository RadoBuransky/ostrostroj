package com.buransky.ostrostroj.app.audio

import java.nio.ByteBuffer

import org.junit.runner.RunWith
import org.scalatest.flatspec.AnyFlatSpec
import org.scalatestplus.junit.JUnitRunner

import scala.util.Random

@RunWith(classOf[JUnitRunner])
class AudioMixerSpec extends AnyFlatSpec {
  behavior of "mix16bit"

  it should "sum two bytes" in {
    val buffer1 = shortsToByteArray(1)
    val buffer2 = shortsToByteArray(-2)

    // Execute
    AudioMixer.mix16bitLe(List(buffer1, buffer2), buffer1.length)

    // Assert
    assert(shortsToByteArray(-1) sameElements buffer1)
  }

  it should "sum two shorts" in {
    val buffer1 = shortsToByteArray(898)
    val buffer2 = shortsToByteArray(-1357)

    // Execute
    AudioMixer.mix16bitLe(List(buffer1, buffer2), buffer1.length)

    // Assert
    assert(shortsToByteArray(-459) sameElements buffer1)
  }

  it should "clip high overflow" in {
    val buffer1 = shortsToByteArray(20000)
    val buffer2 = shortsToByteArray(30000)

    // Execute
    AudioMixer.mix16bitLe(List(buffer1, buffer2), buffer1.length)

    // Assert
    assert(shortsToByteArray(32767) sameElements buffer1)
  }

  it should "clip low overflow" in {
    val buffer1 = shortsToByteArray(-20000)
    val buffer2 = shortsToByteArray(-30000)

    // Execute
    AudioMixer.mix16bitLe(List(buffer1, buffer2), buffer1.length)

    // Assert
    assert(shortsToByteArray(-32768) sameElements buffer1)
  }

  it should "sum 10 random shorts" in {
    val shorts1 = (1 to 10).map(_ => Random.nextInt().toShort)
    val shorts2 = (1 to 10).map(_ => Random.nextInt().toShort)
    val buffer1 = shortsToByteArray(shorts1:_*)
    val buffer2 = shortsToByteArray(shorts2:_*)

    // Execute
    AudioMixer.mix16bitLe(List(buffer1, buffer2), buffer1.length)

    // Assert
    val saturatedSum = shorts1.zip(shorts2).map { ss =>
      val sum = ss._1.toInt + ss._2.toInt
      if (sum > Short.MaxValue)
        Short.MaxValue
      else {
        if (sum < Short.MinValue)
          Short.MinValue
        else
          sum.toShort
      }
    }
    val expected = shortsToByteArray(saturatedSum:_*)
    assert(expected sameElements buffer1)
  }

  private def shortsToByteArray(shorts: Short*): Array[Byte] = {
    val buffer = ByteBuffer.allocate(shorts.length * 2)
    shorts.foreach { s =>
      buffer.put((s & 0xFF).toByte)
      buffer.put(((s >>> 8) & 0xFF).toByte)
    }
    buffer.array()
  }
}
