package com.buransky.ostrostroj.app.audio.impl.input

import com.buransky.ostrostroj.app.audio._
import com.buransky.ostrostroj.app.show.Loop
import javax.sound.sampled.AudioInputStream
import org.slf4j.LoggerFactory

private[audio] class SongInputImpl(loops: Seq[Loop],
                                   masterTrackInputStream: AudioInputStream) extends SongInput {
  import SongInputImpl._

  private var _loopInput: Option[LoopInput] = ???

  override def startLooping(): Unit = synchronized {
    _loopInput match {
      case Some(li) => li.stopDraining()
      case None =>
        val currentPosition: SampleCount = ???
        loopAtPosition(currentPosition) match {
          case Some(loop) =>
            _loopInput = Some(createLoopInput(loop))
            // TODO: Skip master track to the end of the loop
            masterTrackInputStream.skip(???)
          case None => logger.debug(s"No loop at position. [${currentPosition.value}]")
        }
    }
  }

  override def stopLooping(): Unit = synchronized {
    _loopInput.foreach(_.startDraining())
  }

  override def toggleLooping(): Unit = synchronized {
    _loopInput match {
      case Some(li) => li.toggleDraining()
      case None => startLooping()
    }
  }

  override def loopInput: Option[LoopInput] = synchronized { _loopInput }

  override def read(buffer: AudioBuffer): AudioBuffer = synchronized {
    _loopInput match {
      case Some(_) => readFromLoop(buffer)
      case None => readFromMaster(buffer)
    }
  }

  private def loopAtPosition(position: SampleCount): Option[Loop] = ???

  private def createLoopInput(loop: Loop): LoopInput = ???

  private def readFromLoop(buffer: AudioBuffer): AudioBuffer = {
    val result = _loopInput.get.read(buffer)
    if (result.endOfStream) {
      _loopInput.get.close()
      _loopInput = None
    }

    ???
  }

  private def readFromMaster(buffer: AudioBuffer): AudioBuffer = {
    val bytesRead = masterTrackInputStream.read(buffer.byteArray)

    // TODO: End of stream
    ???
  }

  override def close(): Unit = synchronized {
    loopInput.foreach(_.close())
    logger.debug("Song input closed.")
  }
}
private object SongInputImpl {
  private val logger = LoggerFactory.getLogger(classOf[SongInputImpl])
}
