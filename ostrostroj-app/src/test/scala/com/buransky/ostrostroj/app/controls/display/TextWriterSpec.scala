package com.buransky.ostrostroj.app.controls.display

import com.buransky.max7219.LedMatrix
import com.buransky.ostrostroj.app.controls.display.BitmapDisplay.Position
import org.junit.runner.RunWith
import org.mockito.Mockito._
import org.scalatest.flatspec.AnyFlatSpec
import org.scalatestplus.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
class TextWriterSpec extends AnyFlatSpec {
  import TextWriterSpec._

  behavior of "write"

  it should "write \"a\"" in {
    // Prepare
    val expected =
      """..........-
        |..........-
        |..........-
        |...XX.....-
        |..X.X.....-
        |...XX.....-""".stripMargin
    val ledMatrix = mock(classOf[LedMatrix])
    val textWriter = new TextWriter(ledMatrix, totalRowCount)

    // Execute
    textWriter.write("a", Position(2, 2), color = true)

    // Verify
    CanvasSpec.verifyExpectedLedStatus(expected, color = true, ledMatrix)
    verifyNoMoreInteractions(ledMatrix)
  }

  it should "write \"čh7\"" in {
    // Prepare
    val expected =
      """XXXX.XX.-
        |..XX...X-
        |.XXXXX.X-
        |.X.X.X.X-""".stripMargin
    val ledMatrix = mock(classOf[LedMatrix])
    val textWriter = new TextWriter(ledMatrix, totalRowCount)

    // Execute
    textWriter.write("čh7", Position(0, 0), color = false)

    // Verify
    CanvasSpec.verifyExpectedLedStatus(expected, color = false, ledMatrix)
    verifyNoMoreInteractions(ledMatrix)
  }

  it should "write \"Ost\"" in {
    // Prepare
    val expected =
      """.XX.....X..-
        |X..X.XXXXX.-
        |X..X.X..X..-
        |.XX.XX..X..-""".stripMargin
    val ledMatrix = mock(classOf[LedMatrix])
    val textWriter = new TextWriter(ledMatrix, totalRowCount)

    // Execute
    textWriter.write("Ost", Position(0, 0), color = false)

    // Verify
    CanvasSpec.verifyExpectedLedStatus(expected, color = false, ledMatrix)
    verifyNoMoreInteractions(ledMatrix)
  }
}

object TextWriterSpec {
  val totalRowCount = 8
}