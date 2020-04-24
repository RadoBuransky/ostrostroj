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
    verifyExpectedLedStatus(expected, color = true, ledMatrix)
    verifyNoMoreInteractions(ledMatrix)
  }

  it should "write \"čh7\"" in {
    // Prepare
    val expected =
      """XXX..X.XX.-
        |..X..X...X-
        |.XX..XXX.X-
        |.X...X.X.X-""".stripMargin
    val ledMatrix = mock(classOf[LedMatrix])
    val textWriter = new TextWriter(ledMatrix, totalRowCount)

    // Execute
    textWriter.write("čh7", Position(0, 0), color = false)

    // Verify
    verifyExpectedLedStatus(expected, color = false, ledMatrix)
    verifyNoMoreInteractions(ledMatrix)
  }

  private def verifyExpectedLedStatus(expected: String, color: Boolean, ledMatrix: LedMatrix): Unit = {
    var row = totalRowCount - 1
    var column = 0
    expected.foreach {
      case '.' => column += 1
      case 'X' =>
        verify(ledMatrix).setLedStatus(row, column, color)
        column += 1
      case '-' =>
        row -= 1
        column = 0
      case _ =>
    }
  }
}

object TextWriterSpec {
  val totalRowCount = 8
}