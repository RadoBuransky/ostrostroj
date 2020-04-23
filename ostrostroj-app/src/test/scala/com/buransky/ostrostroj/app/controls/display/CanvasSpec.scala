package com.buransky.ostrostroj.app.controls.display

import java.nio.charset.Charset

import org.junit.runner.RunWith
import org.scalatest.flatspec.AnyFlatSpec
import org.scalatestplus.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
class CanvasSpec extends AnyFlatSpec {
  behavior of "write"

  it should "work" in {
    val someText = "abc"
    val charset = Charset.forName("IBM437")
    val data = someText.getBytes(charset)
    assert(data(0) == 97)
    assert(data(1) == 98)
    assert(data(2) == 99)
  }
}
