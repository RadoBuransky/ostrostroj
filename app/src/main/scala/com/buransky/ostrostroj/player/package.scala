package com.buransky.ostrostroj

import javax.sound.midi.MidiMessage

package object player {
  implicit class MidiMessageOps(msg: MidiMessage) {
    val (command, channel): (Int, Option[Int]) = {
      val high = msg.getStatus & 0xF0
      if (high >= 0x80 && high <= 0xE0) {
        (high, Some((msg.getStatus & 0x0F) + 1))
      } else {
        (msg.getStatus, None)
      }
    }
  }
}
