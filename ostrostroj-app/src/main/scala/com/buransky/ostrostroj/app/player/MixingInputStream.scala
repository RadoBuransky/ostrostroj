package com.buransky.ostrostroj.app.player

import java.io.InputStream

import javax.sound.sampled.AudioFormat

class MixingInputStream(upstreams: Iterable[InputStream], audioFormat: AudioFormat) extends InputStream {
  override def read(): Int = ???
}
