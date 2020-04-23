package com.buransky.ostrostroj.app.controls.display

import com.buransky.max7219.LedMatrix
import com.buransky.ostrostroj.app.controls.display.BitmapDisplay.Position

class Canvas(ledMatrix: LedMatrix, totalRowCount: Int) {
  private val textWriter: TextWriter = new TextWriter(ledMatrix, totalRowCount)

  def point(position: Position, color: Boolean): Unit = {
    ledMatrix.setLedStatus(yToRow(position.y), position.x, color)
  }

  def horizontalLine(from: Position, length: Int, color: Boolean): Unit = {
    for (i <- 0 until length) {
      ledMatrix.setLedStatus(yToRow(from.y), from.x + i, color)
    }
  }

  def verticalLine(from: Position, length: Int, color: Boolean): Unit = {
    for (i <- 0 until length) {
      ledMatrix.setLedStatus(yToRow(from.y + i), from.x, color)
    }
  }

  def rectangle(from: Position, to: Position, color: Boolean): Unit = {
    for (row <- from.y to to.y) {
      for (column <- from.x to to.x) {
        ledMatrix.setLedStatus(yToRow(row), column, color)
      }
    }
  }

  def write(text: String, position: Position, color: Boolean): Unit = textWriter.write(text, position, color)

  def draw(bitmap: Vector[Vector[Boolean]]): Unit = {
    for (column <- bitmap.indices) {
      for (row <- bitmap(column).indices) {
        ledMatrix.setLedStatus(yToRow(row), column, bitmap(column)(row))
      }
    }
  }

  /**
   * Converts "y" canvas coordinate to LED matrix "row".
   * @param y Canvas coordinate.
   * @return LED matrix row.
   */
  private def yToRow(y: Int): Int = (totalRowCount - 1) - y
}