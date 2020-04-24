package com.buransky.ostrostroj.app.controls.display

import java.nio.charset.Charset

import com.buransky.max7219.LedMatrix
import com.buransky.ostrostroj.app.controls.display.BitmapDisplay.Position
import org.junit.runner.RunWith
import org.mockito.Mockito._
import org.scalatest.flatspec.AnyFlatSpec
import org.scalatestplus.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
class CanvasSpec extends AnyFlatSpec {
  import CanvasSpec._

  behavior of "point"

  it should "draw a single point" in {
    // Prepare
    val expected =
      """..........-
        |..........-
        |..........-
        |.X........-
        |..........-
        |..........-""".stripMargin
    val ledMatrix = mock(classOf[LedMatrix])
    val canvas = new Canvas(ledMatrix, totalRowCount)

    // Execute
    canvas.point(Position(1, 3), color = true)

    // Verify
    verifyExpectedLedStatus(expected, color = true, ledMatrix)
    verifyNoMoreInteractions(ledMatrix)
  }

  behavior of "horizontalLine"

  it should "draw a horizontal line" in {
    // Prepare
    val expected =
      """..........-
        |.XXX......-
        |..........-
        |..........-
        |..........-
        |..........-""".stripMargin
    val ledMatrix = mock(classOf[LedMatrix])
    val canvas = new Canvas(ledMatrix, totalRowCount)

    // Execute
    canvas.horizontalLine(from = Position(1, 1), length = 3, color = true)

    // Verify
    verifyExpectedLedStatus(expected, color = true, ledMatrix)
    verifyNoMoreInteractions(ledMatrix)
  }

  behavior of "verticalLine"

  it should "draw a vertical line" in {
    // Prepare
    val expected =
      """..........-
        |..........-
        |X.........-
        |X.........-
        |..........-
        |..........-""".stripMargin
    val ledMatrix = mock(classOf[LedMatrix])
    val canvas = new Canvas(ledMatrix, totalRowCount)

    // Execute
    canvas.verticalLine(from = Position(0, 2), length = 2, color = true)

    // Verify
    verifyExpectedLedStatus(expected, color = true, ledMatrix)
    verifyNoMoreInteractions(ledMatrix)
  }

  behavior of "rectangle"

  it should "draw a rectangle" in {
    // Prepare
    val expected =
      """XXXXX.....-
        |XXXXX.....-
        |XXXXX.....-
        |..........-
        |..........-
        |..........-""".stripMargin
    val ledMatrix = mock(classOf[LedMatrix])
    val canvas = new Canvas(ledMatrix, totalRowCount)

    // Execute
    canvas.rectangle(from = Position(0, 0), to = Position(4, 2), color = true)

    // Verify
    verifyExpectedLedStatus(expected, color = true, ledMatrix)
    verifyNoMoreInteractions(ledMatrix)
  }

  behavior of "write"

  it should "write a letter" in {
    // Prepare
    val expected =
      """..........-
        |..........-
        |..........-
        |...XX.....-
        |..X.X.....-
        |...XX.....-""".stripMargin
    val ledMatrix = mock(classOf[LedMatrix])
    val canvas = new Canvas(ledMatrix, totalRowCount)

    // Execute
    canvas.write("a", Position(2, 2), color = true)

    // Verify
    verifyExpectedLedStatus(expected, color = true, ledMatrix)
    verifyNoMoreInteractions(ledMatrix)
  }
}

object CanvasSpec {
  val totalRowCount = 8

  def verifyExpectedLedStatus(expected: String, color: Boolean, ledMatrix: LedMatrix): Unit = {
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