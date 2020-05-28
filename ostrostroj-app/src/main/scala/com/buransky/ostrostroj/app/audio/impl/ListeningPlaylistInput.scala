package com.buransky.ostrostroj.app.audio.impl

import com.buransky.ostrostroj.app.audio._
import org.slf4j.{Logger, LoggerFactory}

import scala.annotation.tailrec

private[audio] class AsyncListeningPlaylistInput(initSong: Int, javaSoundOutput: AudioOutput)
  extends SyncListeningPlaylistInput(initSong, javaSoundOutput) { self =>

  import SyncListeningPlaylistInput._

  private val thread = new Thread {
    override def run(): Unit = self.run()
  }

  @tailrec
  final override def run(): Unit = {
    try {
      super.run()
    }
    catch {
      case _: InterruptedException =>
        logger.info(s"Playlist input thread interrupted by InterruptedException.")
      case t: Throwable =>
        logger.error("Playlist input thread failed!", t)
        throw t
    }
    if (!thread.isInterrupted) {
      run()
    } else {
      logger.info(s"Playlist input thread stopped because it was interrupted.")
    }
  }

  override def close(): Unit = {
    logger.debug(s"Interrupting playlist input thread...")
    thread.interrupt()
  }
}

private[audio] class SyncListeningPlaylistInput(initSong: Int, javaSoundOutput: AudioOutput)
  extends PlaylistInput {
  import SyncListeningPlaylistInput._

  override def run(): Unit = {
    logger.trace("Acquiring semaphore for empty buffers...")
    javaSoundOutput.emptyAvailable.acquire()
    javaSoundOutput.nextEmpty() match {
      case Some(buffer) =>
        logger.trace("Empty buffer dequeued.")

        val lastBufferEnd = javaSoundOutput.bufferingPosition.getOrElse(PlaybackPosition(initSong, SampleCount(0)))
        // TODO: Fill the buffer
        val audioBuffer: AudioBuffer = ???

        javaSoundOutput.write(audioBuffer)
      case None =>
        logger.warn("Semaphore acquired but no empty buffer in the queue.")
    }
  }

  override def close(): Unit = {}
}

private object SyncListeningPlaylistInput {
  val logger: Logger = LoggerFactory.getLogger(classOf[SyncListeningPlaylistInput])
}