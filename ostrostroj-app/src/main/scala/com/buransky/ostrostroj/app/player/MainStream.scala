package com.buransky.ostrostroj.app.player

import com.buransky.ostrostroj.app.show.Song
import org.slf4j.LoggerFactory

class MainStream(song: Song) extends AutoCloseable {
  def read(result: Array[Byte]): Int = ???

  def startLooping(): Unit = ???
  def stopLooping(): Unit = ???

  override def close(): Unit = ???
}

object MainStream {
  private val logger = LoggerFactory.getLogger(classOf[MainStream])
}