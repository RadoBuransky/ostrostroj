package com.buransky.ostrostroj.app.player

import java.io.InputStream

class MutingInputStream(upstream: InputStream) extends InputStream {
  def mute(): Unit = ???
  def unmute(): Unit = ???

  override def read(): Int = ???
}
