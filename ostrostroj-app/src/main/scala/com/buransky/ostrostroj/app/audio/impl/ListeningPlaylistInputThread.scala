package com.buransky.ostrostroj.app.audio.impl

import com.buransky.ostrostroj.app.audio.{AudioBuffer, JavaSoundOutput, PlaylistInput}
import org.slf4j.LoggerFactory

import scala.annotation.tailrec

private[audio] class ListeningPlaylistInputThread(initSong: Int, javaSoundOutput: JavaSoundOutput)
  extends PlaylistInput with AutoCloseable {
  import ListeningPlaylistInputThread._

  private val thread = new Thread {
    @tailrec
    override def run(): Unit = {
      try {
        logger.trace("Acquiring semaphore for empty buffers...")
        javaSoundOutput.emptyAvailable.acquire()
        javaSoundOutput.nextEmpty() match {
          case Some(buffer) =>
            logger.trace("Empty buffer dequeued.")

            // TODO: Fill the buffer
            javaSoundOutput.bufferingPosition
            val audioBuffer: AudioBuffer = ???

            javaSoundOutput.write(audioBuffer)
          case None =>
            logger.warn("Semaphore acquired but no empty buffer in the queue.")
        }
      }
      catch {
        case _: InterruptedException =>
          logger.info(s"Playlist input thread interrupted by InterruptedException.")
        case t: Throwable =>
          logger.error("Playlist input thread failed!", t)
          throw t
      }
      if (!isInterrupted) {
        run()
      } else {
        logger.info(s"Playlist input thread stopped because it was interrupted.")
      }
    }
  }

  override def close(): Unit = {
    thread.interrupt()
  }
}

private object ListeningPlaylistInputThread {
  private val logger = LoggerFactory.getLogger(classOf[ListeningPlaylistInputThread])
}