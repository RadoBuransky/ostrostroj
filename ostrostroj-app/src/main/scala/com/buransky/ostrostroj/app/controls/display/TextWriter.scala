package com.buransky.ostrostroj.app.controls.display

import java.awt.image.BufferedImage
import java.nio.ByteBuffer
import java.nio.charset.Charset

import com.buransky.max7219.LedMatrix
import com.buransky.ostrostroj.app.controls.display.BitmapDisplay.Position
import javax.imageio.ImageIO

import scala.annotation.tailrec

class TextWriter(ledMatrix: LedMatrix, totalRowCount: Int) {
  import TextWriter._
  private val memFont: Font = FontXml(fontFileResourceName)
  private val chars: Map[Char, FontChar] = decodeChars(memFont)
  private val fontImage: BufferedImage = readFontImage(memFont.file)

  def write(text: String, position: Position, color: Boolean): Unit = {
    val fontChars = text.toList.map { c =>
      chars.get(c) match {
        case Some(fontChar) => fontChar
        case _ => chars('?')
      }
    }
    write(fontChars, position, color)
  }

  @tailrec
  private def write(fontChars: List[FontChar], position: Position, color: Boolean): Unit = {
    fontChars match {
      case head :: tail =>
        for (x <- 0 to 3) {
          for (y <- 0 to 3) {
            if (fontImage.getRGB(head.x + x, head.y + y) != 0) {
              ledMatrix.setLedStatus(Canvas.yToRow(position.y + y, totalRowCount), position.x + x, color)
            }
          }
        }
        // TODO: Kerning
        write(tail, Position(x = position.x + head.xadvance, y = position.y), color)
      case _ =>
    }
  }

  private def decodeChars(font: Font): Map[Char, FontChar] = {
    val charset = Charset.forName(font.charset)
    font.chars.map { fontChar =>
      val unicodeChar = charset.decode(ByteBuffer.wrap(Array(fontChar.id.toByte))).get(0)
      unicodeChar -> fontChar
    }.toMap
  }

  private def readFontImage(resourceName: String): BufferedImage = {
    val resourceStream = ClassLoader.getSystemResourceAsStream(resourceName)
    try {
      ImageIO.read(resourceStream)
    } finally {
      resourceStream.close()
    }
  }
}

object TextWriter {
  private val fontFileResourceName = "mem.fnt"
}