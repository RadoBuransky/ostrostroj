package com.buransky.ostrostroj.app.controls.display

import com.buransky.max7219.LedMatrix
import com.buransky.ostrostroj.app.controls.display.BitmapDisplay.Position

class Canvas(ledMatrix: LedMatrix) {
  def point(position: Position, color: Boolean): Unit = {
    ledMatrix.setLedStatus(position.row, position.column, color)
  }

  def horizontalLine(from: Position, length: Int, color: Boolean): Unit = {
    for (i <- 0 until length) {
      ledMatrix.setLedStatus(from.row, from.column + i, color)
    }
  }

  def verticalLine(from: Position, length: Int, color: Boolean): Unit = {
    for (i <- 0 until length) {
      ledMatrix.setLedStatus(from.row + i, from.column, color)
    }
  }

  def rectangle(from: Position, to: Position, color: Boolean): Unit = {
    for (row <- from.row to to.row) {
      for (column <- from.column to to.column) {
        ledMatrix.setLedStatus(row, column, color)
      }
    }
  }

  def write(text: String, position: Position, color: Boolean): Unit = ???

  def draw(bitmap: Vector[Vector[Boolean]]): Unit = {
    for (column <- bitmap.indices) {
      for (row <- bitmap(column).indices) {
        ledMatrix.setLedStatus(row, column, bitmap(column)(row))
      }
    }
  }
}
