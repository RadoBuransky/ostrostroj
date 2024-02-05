package com.buransky.ostrostroj.player

import com.buransky.ostrostroj.player.midi.MidiCommands

import javax.sound.sampled._

class Player(clips: Vector[Clip]) extends MidiCommands {
  private var index = 0

  override def start(): Unit = {
    clips(index).start()
  }

  override def stop(): Unit = {
    clips(index).stop()
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
}

object Player {
  def apply(): Player = {
    val audioFormat: AudioFormat = ???
    val clip = AudioSystem.getLine(new DataLine.Info(classOf[Clip], audioFormat)).asInstanceOf[Clip]
    clip.open()
    clip.loop(Clip.LOOP_CONTINUOUSLY)
    new Player(Vector(clip))
  }
}