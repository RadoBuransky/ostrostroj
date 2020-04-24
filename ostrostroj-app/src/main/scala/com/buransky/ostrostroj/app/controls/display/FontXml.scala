package com.buransky.ostrostroj.app.controls.display

import scala.xml.{Elem, Node, XML}

case class Font(charset: String, file: String, chars: Iterable[FontChar], kernings: Iterable[Kerning])
case class FontChar(id: Int, x: Int, y: Int, width: Int, height: Int, xoffset: Int, yoffset: Int, xadvance: Int)
case class Kerning(firstId: Int, secondId: Int, amount: Int)

/**
 * http://www.angelcode.com/products/bmfont/doc/file_format.html
 */
object FontXml {
  def apply(resourceName: String): Font = {
    val fontFile = readFontFile(resourceName)
    val charset = fontFile \ "info" \@ "charset"
    val file = fontFile \ "pages" \ "page" \@ "file"
    val chars = (fontFile  \ "chars" \ "char").map(processCharNode)
    val kernings = (fontFile \ "kernings" \ "kerning").map(processKerningNode)
    Font(charset, file, chars, kernings)
  }

  private def processKerningNode(kerningNode: Node): Kerning = {
    val firstId = (kerningNode \@ "first").toInt
    val secondId = (kerningNode \@ "second").toInt
    val amount = (kerningNode \@ "amount").toInt
    Kerning(firstId, secondId, amount)
  }

  private def processCharNode(charNode: Node): FontChar = {
    val id = (charNode \@ "id").toInt
    val x = (charNode \@ "x").toInt
    val y = (charNode \@ "y").toInt
    val width = (charNode \@ "width").toInt
    val height = (charNode \@ "height").toInt
    val xoffset = (charNode \@ "xoffset").toInt
    val yoffset = (charNode \@ "yoffset").toInt
    val xadvance = (charNode \@ "xadvance").toInt
    FontChar(id, x, y, width, height, xoffset, yoffset, xadvance)
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
