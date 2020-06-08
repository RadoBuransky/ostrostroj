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
  private var _done: Boolean = false

  override def startLooping(): Unit = synchronized {
    logger.debug("Start looping.")
    _loopInput match {
      case Some(li) => li.stopDraining()
      case None =>
        song.loopAtPosition(masterTrackPosition) match {
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
    _loopInput match {
      case Some(_) => readFromLoop(buffer)
      case None => readFromMaster(buffer)
    }
  }

  override def close(): Unit = synchronized {
    _loopInput.foreach(_.close())
    _loopInput = None
    logger.debug("Song input closed.")
  }

  override def status: SongStatus = SongStatus(
    song = song,
    loopStatus = _loopInput.map(_.status),
    position = _loopInput.map(_.status.position).getOrElse(masterTrackPosition),
    done =_done
  )

  private def skipToEndOfLoop(audioInputStream: AudioInputStream, loop: Loop): Unit = {
    val frameSize = audioInputStream.getFormat.getFrameSize
    val bytesToSkip = (loop.endExclusive - masterTrackPosition.value)*frameSize
    val bytesSkipped = audioInputStream.skip(bytesToSkip)
    masterTrackPosition += FrameCount(bytesSkipped.toInt/frameSize)
    if (bytesSkipped != bytesToSkip) {
      logger.warn(s"Unexpected bytes skipped! [$bytesToSkip, $bytesSkipped]")
    } else {
      logger.debug(s"Master track skipped. [$bytesSkipped]")
    }
  }

  private def readFromLoop(buffer: AudioBuffer): AudioBuffer = {
    logger.trace(s"Reading from loop. [${buffer.capacity}")
    val result = _loopInput.get.read(buffer)
    if (result.endOfStream) {
      logger.debug("Loop draining completed.")
      _loopInput.get.close()
      _loopInput = None
      if (result.size.value == 0) {
        read(buffer)
      } else {
        result.copy(endOfStream = false)
      }
    } else {
      result
    }
  }

  private def readFromMaster(buffer: AudioBuffer): AudioBuffer = {
    logger.trace(s"Reading from master track. [${buffer.capacity}, $masterTrackPosition]")
    val bytesRead = masterTrackInputStream.read(buffer.byteArray)
    if (bytesRead <= 0) {
      _done = true
      logger.debug(s"End of master track.")
      buffer.copy(position = FrameCount(0), limit = FrameCount(0), endOfStream = true)
    } else {
      logger.trace(s"Master track read. [$bytesRead]")
      val framesRead = FrameCount(bytesRead / masterTrackInputStream.getFormat.getFrameSize)
      if (framesRead.value == 0) {
        logger.warn(s"Weird number of frames read from master track! [${framesRead}]")
      }
      masterTrackPosition += framesRead
      buffer.copy(position = FrameCount(0), limit = framesRead)
    }
  }
}
private object SongInputImpl {
  private val logger = LoggerFactory.getLogger(classOf[SongInputImpl])
}
