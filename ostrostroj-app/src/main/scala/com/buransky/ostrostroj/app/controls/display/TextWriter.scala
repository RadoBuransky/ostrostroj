package com.buransky.ostrostroj.app.controls.display

import java.awt.image.BufferedImage
import java.nio.ByteBuffer
import java.nio.charset.Charset

import com.buransky.max7219.LedMatrix
import com.buransky.ostrostroj.app.controls.display.BitmapDisplay.Position
import javax.imageio.ImageIO

import scala.annotation.tailrec
import scala.xml.{Elem, Node, XML}

class TextWriter(ledMatrix: LedMatrix, totalRowCount: Int) {
  import TextWriter._
  private[display] val chars: Map[Char, FontChar] = loadChars(fontFileResourceName)
  private[display] val fontImage: BufferedImage = readFontImage(fontImageResourceName)

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
              ledMatrix.setLedStatus(yToRow(position.y + y), position.x + x, color)
            }
          }
        }
        // TODO: Kerning
        write(tail, Position(x = position.x + head.xadvance, y = position.y), color)
      case _ =>
    }
  }

  /**
   * Converts "y" canvas coordinate to LED matrix "row".
   * @param y Canvas coordinate.
   * @return LED matrix row.
   */
  private def yToRow(y: Int): Int = (totalRowCount - 1) - y

  private def loadChars(fontFileResourceName: String): Map[Char, FontChar] = {
    val ibm437Charset = Charset.forName("IBM437")
    val charNodes = readFontFile(fontFileResourceName) \ "chars" \ "char"
    charNodes.map(processCharNode(_, ibm437Charset)).toMap
  }

  private def processCharNode(charNode: Node, ibm437Charset: Charset) = {
    val ibm437Char = (charNode \@ "id").toInt
    val char = ibm437Charset.decode(ByteBuffer.wrap(Array(ibm437Char.toByte))).get(0)
    val x = (charNode \@ "x").toInt
    val y = (charNode \@ "y").toInt
    val xoffset = (charNode \@ "xoffset").toInt
    val yoffset = (charNode \@ "yoffset").toInt
    val xadvance = (charNode \@ "xadvance").toInt
    char -> FontChar(char, ibm437Char, x, y, xoffset, yoffset, xadvance)
  }

  private def readFontImage(resourceName: String): BufferedImage = {
    val resourceStream = ClassLoader.getSystemResourceAsStream(resourceName)
    try {
      ImageIO.read(resourceStream)
    } finally {
      resourceStream.close()
    }
  }

  private def readFontFile(resourceName: String): Elem = {
    val resourceStream = ClassLoader.getSystemResourceAsStream(resourceName)
    try {
      XML.load(resourceStream)
    } finally {
      resourceStream.close()
    }
  }
}

object TextWriter {
  private val fontFileResourceName = "font.fnt"
  private val fontImageResourceName = "font.png"
}

private case class FontChar(unicodeChar: Char, ibm437Char: Int, x: Int, y: Int, xoffset: Int, yoffset: Int,
                            xadvance: Int)