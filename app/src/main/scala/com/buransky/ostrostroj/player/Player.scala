package com.buransky.ostrostroj.player

import com.buransky.ostrostroj.player.audio.JackAudio
import com.buransky.ostrostroj.player.midi.MidiCommands
import org.slf4j.LoggerFactory

import javax.sound.sampled._

class Player(clips: Vector[Clip], jackAudio: JackAudio) extends MidiCommands with AutoCloseable {
  private var index = 0

  override def start(): Unit = {
//    clips(index).start()
  }

  override def stop(): Unit = {
//    clips(index).stop()
  }

  override def continue(): Unit = {
    start()
  }

  override def programChange(num: Int): Unit = {
    if (num != index && num >= 0 && num < clips.size) {
      stop()
      index = num
      start()
    }
  }

  override def tick(): Unit = {
  }

  override def close(): Unit = {
    jackAudio.close()
  }
}

object Player {
  private val log = LoggerFactory.getLogger(classOf[Player])
  def apply(): Player = {
    new Player(Vector.empty, JackAudio())
  }
}