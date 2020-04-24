package com.buransky.ostrostroj.app.controls.display

import java.awt.image.BufferedImage
import java.nio.ByteBuffer
import java.nio.charset.Charset

import com.buransky.max7219.LedMatrix
import com.buransky.ostrostroj.app.controls.display.BitmapDisplay.Position
import javax.imageio.ImageIO

import scala.annotation.tailrec

/**
 * Thanks! https://github.com/oddoid/mem-font
 */
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
        writeChar(head, position, color)
        val kerning = kerningAmount(head, tail)
        write(tail, Position(x = position.x + head.xadvance + kerning, y = position.y), color)
      case _ =>
    }
  }

  private def kerningAmount(firstChar: FontChar, remainingChars: List[FontChar]): Int = {
    remainingChars match {
      case secondChar :: _ =>
        memFont
          .kernings
          .find(k => k.firstId == firstChar.id && k.secondId == secondChar.id)
          .map(_.amount)
          .getOrElse(0)
      case Nil => 0
    }
  }

  private def writeChar(fontChar: FontChar, position: Position, color: Boolean): Unit = {
    for (x <- 0 until fontChar.width) {
      for (y <- 0 to fontChar.height) {
        if (fontImage.getRGB(fontChar.x + x, fontChar.y + y) != 0) {
          ledMatrix.setLedStatus(Canvas.yToRow(position.y + y, totalRowCount), position.x + x, color)
        }
      }
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