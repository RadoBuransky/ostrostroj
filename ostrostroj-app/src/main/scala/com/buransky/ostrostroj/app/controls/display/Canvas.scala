package com.buransky.ostrostroj.app.controls.display

import com.buransky.max7219.LedMatrix
import com.buransky.ostrostroj.app.controls.display.BitmapDisplay.Position

class Canvas(ledMatrix: LedMatrix, totalRowCount: Int) {
  import Canvas._

  private val textWriter: TextWriter = new TextWriter(ledMatrix, totalRowCount)

  def point(position: Position, color: Boolean): Unit = {
    ledMatrix.setLedStatus(yToRow(position.y, totalRowCount), position.x, color)
  }

  def horizontalLine(from: Position, length: Int, color: Boolean): Unit = {
    for (i <- 0 until length) {
      ledMatrix.setLedStatus(yToRow(from.y, totalRowCount), from.x + i, color)
    }
  }

  def verticalLine(from: Position, length: Int, color: Boolean): Unit = {
    for (i <- 0 until length) {
      ledMatrix.setLedStatus(yToRow(from.y + i, totalRowCount), from.x, color)
    }
  }

  def rectangle(from: Position, to: Position, color: Boolean): Unit = {
    for (row <- from.y to to.y) {
      for (column <- from.x to to.x) {
        ledMatrix.setLedStatus(yToRow(row, totalRowCount), column, color)
      }
    }
  }

  def write(text: String, position: Position, color: Boolean): Unit = textWriter.write(text, position, color)
}

object Canvas {
  /**
   * Converts "y" canvas coordinate to LED matrix "row".
   * @param y Canvas coordinate.
   * @return LED matrix row.
   */
  def yToRow(y: Int, totalRowCount: Int): Int = (totalRowCount - 1) - y
}