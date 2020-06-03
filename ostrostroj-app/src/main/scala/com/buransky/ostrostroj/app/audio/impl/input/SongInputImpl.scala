package com.buransky.ostrostroj.app.audio.impl.input

import com.buransky.ostrostroj.app.audio._
import com.buransky.ostrostroj.app.show.{Loop, Song}
import javax.sound.sampled.AudioInputStream
import org.slf4j.LoggerFactory

private[audio] class SongInputImpl(song: Song,
                                   masterTrackInputStream: AudioInputStream,
                                   loopInputFactory: (Loop, FrameCount) => LoopInput) extends SongInput {
  import SongInputImpl._

  private var _loopInput: Option[LoopInput] = None
  private var masterTrackPosition: FrameCount = FrameCount(0)

  override def startLooping(): Unit = synchronized {
    logger.debug("Start looping.")
    _loopInput match {
      case Some(li) => li.stopDraining()
      case None =>
        loopAtPosition(masterTrackPosition) match {
          case Some(loop) =>
            _loopInput = Some(loopInputFactory(loop, masterTrackPosition))
            skipToEndOfLoop(masterTrackInputStream, loop)
          case None => logger.debug(s"No loop at position. [${masterTrackPosition.value}]")
        }
    }
  }

  override def stopLooping(): Unit = synchronized {
    logger.debug("Stop looping.")
    _loopInput.foreach(_.startDraining())
  }

  override def toggleLooping(): Unit = synchronized {
    logger.debug("Toggle looping.")
    _loopInput match {
      case Some(li) => li.toggleDraining()
      case None => startLooping()
    }
  }

  override def loopInput: Option[LoopInput] = synchronized { _loopInput }

  override def read(buffer: AudioBuffer): AudioBuffer = synchronized {
    logger.trace("Toggle looping.")
    _loopInput match {
      case Some(_) => readFromLoop(buffer)
      case None => readFromMaster(buffer)
    }
  }

  private def skipToEndOfLoop(audioInputStream: AudioInputStream, loop: Loop): Unit = {
    val bytesToSkip = (loop.endExclusive - masterTrackPosition.value)*audioInputStream.getFormat.getFrameSize
    val bytesSkipped = audioInputStream.skip(bytesToSkip)
    if (bytesSkipped != bytesToSkip) {
      logger.warn(s"Unexpected bytes skipped! [$bytesToSkip, $bytesSkipped]")
    } else {
      logger.debug(s"Master track skipped. [$bytesSkipped]")
    }
  }

  private def loopAtPosition(position: FrameCount): Option[Loop] =
    song.loops.find(l => (position.value >= l.start) && (position.value < l.endExclusive))

  private def readFromLoop(buffer: AudioBuffer): AudioBuffer = {
    val result = _loopInput.get.read(buffer)
    if (result.endOfStream) {
      _loopInput.get.close()
      _loopInput = None
      logger.debug("Loop draining completed.")
    }
    result
  }

  private def readFromMaster(buffer: AudioBuffer): AudioBuffer = {
    val bytesRead = masterTrackInputStream.read(buffer.byteArray)
    if (bytesRead == -1) {
      logger.debug(s"End of master track.")
      buffer.copy(position = FrameCount(0), limit = FrameCount(0), endOfStream = true)
    } else {
      logger.debug(s"Master track read. [$bytesRead]")
      val framesRead = FrameCount(bytesRead / buffer.frameSize)
      masterTrackPosition += framesRead
      buffer.copy(position = FrameCount(0), limit = framesRead)
    }
  }

  override def close(): Unit = synchronized {
    loopInput.foreach(_.close())
    logger.debug("Song input closed.")
  }

  override def status: SongStatus = SongStatus(
    song = song,
    loopStatus = _loopInput.map(_.status),
    position = _loopInput.map(_.status.position).getOrElse(masterTrackPosition)
  )
}
private object SongInputImpl {
  private val logger = LoggerFactory.getLogger(classOf[SongInputImpl])
}
