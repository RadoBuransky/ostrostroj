package com.buransky.ostrostroj.app.player

import com.buransky.ostrostroj.app.show.{Loop, Song}
import org.slf4j.LoggerFactory

case class LooperReadResult(bytesRead: Int, masterSkip: Int)

class Looper private (song: Song,
                      currentLoop: Option[Loop],
                      currentLevel: LooperLevel,
                      levelBuffers: Seq[LooperLevelBuffer]) {
  import Looper._

  private var looperPosition: Int = -1

  def this(song: Song) {
    this(song, None, LooperLevel(0, Seq.empty), Seq.empty)
  }

  def read(buffer: Array[Byte], masterStreamPosition: Int): LooperReadResult = {
    currentLoop match {
      case Some(loop) =>
        val bytesRead = 0
        // TODO: Mix levels
        LooperReadResult(bytesRead, loop.endExclusive - masterStreamPosition)
      case None =>
        // TODO: Empty buffers
        val bytesRead = 0
        LooperReadResult(bytesRead, 0)
    }
  }

  def startLooping(masterStreamPosition: Long): Looper = {
    currentLoop match {
      case Some(_) => this
      case None =>
        loopAtPos(masterStreamPosition) match {
          case Some(loop) =>
            // TODO: Load loops here
            val buffers = ???
            new Looper(song, Some(loop), getLevel(0), buffers)
          case None => this
        }
    }
  }
  def stopLooping(): Looper = new Looper(song, None, currentLevel, levelBuffers)
  def harder(): Looper = setLevel(currentLevel.level + 1)
  def softer(): Looper = setLevel(currentLevel.level - 1)

  private def loopAtPos(position: Long): Option[Loop] =
    song.loops.find(loop => (loop.start <= position && loop.endExclusive > position))

  private def setLevel(newLevel: Int): Looper = {
    currentLoop match {
      case Some(loop) =>
        val min = loop.tracks.map(_.level).min
        val max = loop.tracks.map(_.level).max
        if (newLevel < min || newLevel > max) {
          this
        } else {
          new Looper(song, currentLoop, getLevel(newLevel), levelBuffers)
        }
      case None => this
    }
  }

  private def getLevel(value: Int): LooperLevel = {
    val buffers = levelBuffers.sortBy(b => Math.abs(b.level - value)).toList match {
      case h1 :: tail =>
        if (h1.level == value) {
          Seq(h1)
        } else {
          tail match {
            case h2 :: _ => Seq(h1, h2)
            case Nil =>
              logger.warn(s"This should never happen! [$value, ${h1.level}]")
              Seq(h1)
          }
        }
      case Nil => Seq.empty
    }
    LooperLevel(value, buffers)
  }
}

object Looper {
  private val logger = LoggerFactory.getLogger(classOf[Looper])
}

private case class LooperLevel(level: Int, buffers: Seq[LooperLevelBuffer])
private case class LooperLevelBuffer(level: Int, buffer: Array[Byte])
