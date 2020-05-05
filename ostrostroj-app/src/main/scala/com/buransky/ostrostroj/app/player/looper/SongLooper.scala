package com.buransky.ostrostroj.app.player.looper

import com.buransky.ostrostroj.app.show.{Loop, Song}
import javax.sound.sampled.AudioFormat

class SongLooper(song: Song, audioFormat: AudioFormat) {
  private var loopLooper: Option[LoopLooper] = None

  def read(buffer: Array[Byte], masterStreamPosition: Int): LooperReadResult = synchronized {
    loopLooper match {
      case Some(l) => l.read(buffer, masterStreamPosition)
      case None => LooperReadResult(-1, 0)
    }
  }

  def startLooping(masterStreamPosition: Int): Unit = synchronized {
    loopLooper match {
      case Some(l) => l.stopDraining()
      case None => loopLooper = loopAtPosition(masterStreamPosition).map(new LoopLooper(_, audioFormat))
    }
  }

  def stopLooping(): Unit = synchronized {
    loopLooper.foreach(_.startDraining())
  }

  def harder(): Unit = synchronized { loopLooper.foreach(_.harder()) }
  def softer(): Unit = synchronized { loopLooper.foreach(_.softer()) }

  private def loopAtPosition(position: Int): Option[Loop] =
    song.loops.find(l => l.start <= position && l.endExclusive > position)
}