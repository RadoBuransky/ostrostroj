package com.buransky.ostrostroj.app.audio.impl.input

import com.buransky.ostrostroj.app.audio.{AudioBuffer, LoopInput, SampleCount}
import com.buransky.ostrostroj.app.show.Track
import org.slf4j.LoggerFactory

private[audio] class LoopInputImpl(tracks: Seq[Track],
                                   trackAudioBuffers: Seq[AudioBuffer],
                                   startingPosition: SampleCount) extends LoopInput {
  import LoopInputImpl._

  private var level = 0
  private val minLevel = tracks.map(_.rangeMin).min
  private val maxLevel = tracks.map(_.rangeMax).max
  private var isDraining = false
  private var position: SampleCount = startingPosition

  override def harder(): Unit = synchronized {
    if (level < maxLevel) {
      level += 1
      logger.debug(s"Harder level. [$level]")
    }
  }

  override def softer(): Unit = synchronized {
    if (level > minLevel) {
      level -= 1
      logger.debug(s"Softer level. [$level]")
    }
  }

  override def read(buffer: AudioBuffer): AudioBuffer = synchronized {
    ???
  }

  override def close(): Unit = {}

  override def startDraining(): Unit = synchronized {
    isDraining = true
  }

  override def stopDraining(): Unit = synchronized {
    isDraining = false
  }

  override def toggleDraining(): Unit = synchronized {
    isDraining = !isDraining
  }
}

private object LoopInputImpl {
  private val logger = LoggerFactory.getLogger(classOf[LoopInputImpl])
}