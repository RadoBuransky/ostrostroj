package com.buransky.ostrostroj.app.audio

import org.junit.runner.RunWith
import org.scalatest.flatspec.AnyFlatSpec
import org.scalatestplus.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
class FrameCountSpec extends AnyFlatSpec {
  behavior of "+"

  it should "sum two instances" in {
    assert(FrameCount(1) + FrameCount(2) == FrameCount(3))
  }

  it should "increment an instances" in {
    var frameCount = FrameCount(1)
    frameCount += FrameCount(2)
    assert(frameCount == FrameCount(3))
  }

  behavior of "-"

  it should "subtract two instances" in {
    assert(FrameCount(1) - FrameCount(2) == FrameCount(-1))
  }

  it should "decrement an instances" in {
    var frameCount = FrameCount(1)
    frameCount -= FrameCount(2)
    assert(frameCount == FrameCount(-1))
  }
}
