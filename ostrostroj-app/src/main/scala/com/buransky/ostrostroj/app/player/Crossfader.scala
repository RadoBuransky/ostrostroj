package com.buransky.ostrostroj.app.player

import javax.sound.sampled.AudioFormat

class Crossfader(length: Long, audioFormat: AudioFormat) {
  def crossfade(track1: Iterable[Byte], track2: Iterable[Byte], startPos: Long): Unit = ???
}
