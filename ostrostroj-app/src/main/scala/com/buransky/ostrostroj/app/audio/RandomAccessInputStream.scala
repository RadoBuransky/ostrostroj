package com.buransky.ostrostroj.app.audio

import java.io.{DataInput, InputStream, RandomAccessFile}
import java.nio.file.Path

class RandomAccessInputStream(path: Path) extends InputStream with DataInput {
  private val file: RandomAccessFile = new RandomAccessFile(path.toFile, "r")

  def filePointer: Long = file.getFilePointer
  def seek(pos: Long): Unit = file.seek(pos)

  // InputStream interface:
  override def read(): Int = file.read()
  override def read(b: Array[Byte]): Int = file.read(b)
  override def read(b: Array[Byte], off: Int, len: Int): Int = file.read(b, off, len)
  override def skip(n: Long): Long = file.skipBytes(n.toInt)
  override def reset(): Unit = {}
  override def mark(readlimit: Int): Unit = {}
  override def markSupported(): Boolean = false
  override def close(): Unit = {
    super.close()
    file.close()
  }

  // DataInput interface:
  override def readFully(b: Array[Byte]): Unit = file.readFully(b)
  override def readFully(b: Array[Byte], off: Int, len: Int): Unit = file.readFully(b, off, len)
  override def skipBytes(n: Int): Int = file.skipBytes(n)
  override def readBoolean(): Boolean = file.readBoolean()
  override def readByte(): Byte = file.readByte()
  override def readUnsignedByte(): Int = file.readUnsignedByte()
  override def readShort(): Short = file.readShort()
  override def readUnsignedShort(): Int = file.readUnsignedShort()
  override def readChar(): Char = file.readChar()
  override def readInt(): Int = file.readInt()
  override def readLong(): Long = file.readLong()
  override def readFloat(): Float = file.readFloat()
  override def readDouble(): Double = file.readDouble()
  override def readLine(): String = file.readLine()
  override def readUTF(): String = file.readUTF()
}

object RandomAccessInputStream {
  def apply(path: Path): RandomAccessInputStream = new RandomAccessInputStream(path)
}