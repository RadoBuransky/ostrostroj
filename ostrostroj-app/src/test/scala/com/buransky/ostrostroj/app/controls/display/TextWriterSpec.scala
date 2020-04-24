package com.buransky.ostrostroj.app.controls.display

import com.buransky.max7219.LedMatrix
import com.buransky.ostrostroj.app.controls.display.BitmapDisplay.Position
import org.junit.runner.RunWith
import org.mockito.Mockito._
import org.scalatest.flatspec.AnyFlatSpec
import org.scalatestplus.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
class TextWriterSpec extends AnyFlatSpec {
  behavior of "write"

  it should "write abc" in {
    // Prepare
    val ledMatrix = mock(classOf[LedMatrix])
    val textWriter = new TextWriter(ledMatrix, 8)

    // Execute
    textWriter.write("a", Position(2, 2), color = true)

    // Verify
    verify(ledMatrix).setLedStatus(4, 3, true)
    verify(ledMatrix).setLedStatus(4, 4, true)
    verify(ledMatrix).setLedStatus(3, 2, true)
    verify(ledMatrix).setLedStatus(3, 4, true)
    verify(ledMatrix).setLedStatus(2, 3, true)
    verify(ledMatrix).setLedStatus(2, 4, true)
    verifyNoMoreInteractions(ledMatrix)
  }
}
