package com.buransky.ostrostroj.app.player

import java.nio.ByteBuffer

import akka.actor.typed.scaladsl.{AbstractBehavior, ActorContext, Behaviors}
import akka.actor.typed.{Behavior, PostStop, Signal}
import com.buransky.ostrostroj.app.common.OstrostrojException
import com.buransky.ostrostroj.app.player.PlaylistPlayer._
import com.buransky.ostrostroj.app.player.looper.SongPlayer
import com.buransky.ostrostroj.app.show.Playlist
import javax.sound.sampled.{FloatControl, Mixer, SourceDataLine}
import org.slf4j.LoggerFactory

import scala.concurrent.Future
import scala.util.{Failure, Success}

class PlaylistPlayerBehavior private (val playlist: Playlist,
                                      val songIndex: Int,
                                      val songPlayer: SongPlayer,
                                      val mixer: Mixer,
                                      val sourceDataLine: SourceDataLine,
                                      val gainControl: FloatControl,
                                      ctx: ActorContext[Command]) extends AbstractBehavior[Command](ctx) {
  import PlaylistPlayerBehavior._
  import ctx.executionContext

  def this(playlist: Playlist, songIndex: Int, mixer: Mixer, sourceDataLine: SourceDataLine, gainControl: FloatControl,
           ctx: ActorContext[Command]) {
    this(playlist, songIndex, new SongPlayer(playlist.songs(songIndex), sourceDataLine.getBufferSize), mixer,
      sourceDataLine, gainControl, ctx)
  }
  logger.debug(s"Playlist player created. [$songIndex]")

  override def onMessage(msg: Command): Behavior[Command] = msg match {
    case Play =>
      sourceDataLine.start()
      ctx.self ! ReadNextBuffer
      Behaviors.same
    case Pause =>
      sourceDataLine.stop()
      Behaviors.same
    case StartLooping =>
      Future {
        songPlayer.startLooping()
      }
      Behaviors.same
    case StopLooping =>
      songPlayer.stopLooping()
      Behaviors.same
    case Harder =>
      songPlayer.harder()
      Behaviors.same
    case Softer =>
      songPlayer.softer()
      Behaviors.same
    case NextSong if songIndex >= playlist.songs.length - 1 => Behaviors.same
    case NextSong => new PlaylistPlayerBehavior(playlist, songIndex + 1, mixer, sourceDataLine, gainControl, ctx)
    case VolumeUp =>
      changeVolume(+1)
      Behaviors.same
    case VolumeDown =>
      changeVolume(-1)
      Behaviors.same
    case GetStatus(replyTo) =>
      replyTo ! currentPlayerStatus()
      Behaviors.same
    case ReadNextBuffer =>
      readNextBuffer()
      Behaviors.same
    case FutureFailed(t) => throw t
    case NoOp => Behaviors.same
  }

  override def onSignal: PartialFunction[Signal, Behavior[Command]] = {
    case PostStop =>
      sourceDataLine.close()
      mixer.close()
      songPlayer.close()
      logger.info("Playlist player closed.")
      Behaviors.stopped
  }

  def currentPlayerStatus(): PlayerStatus = {
    val songDuration = framePositionToDuration(songPlayer.fileFormat.getFrameLength,
      songPlayer.fileFormat.getFormat.getSampleRate)
    val songPosition = framePositionToDuration(songPlayer.streamPosition / songPlayer.fileFormat.getFormat.getFrameSize,
      songPlayer.fileFormat.getFormat.getSampleRate)
    PlayerStatus(playlist, songIndex, songDuration, songPosition, songPlayer.loopStatus, sourceDataLine.isRunning,
      gainControl.getValue)
  }

  def readNextBuffer(): Unit = {
    val result = Future {
      val buffer = songPlayer.fillBuffer()
      if (buffer.limit() < 1) {
        logger.debug(s"Playback of song finished. Automatically proceeding to the next song. [$songIndex]")
        NextSong
      } else {
        val bytesWritten = writeBuffer(buffer)
        buffer.position(buffer.position() + bytesWritten)
        if (bytesWritten == buffer.limit()) {
          ReadNextBuffer
        } else {
          logger.debug(s"$bytesWritten bytes written but ${buffer.limit() - buffer.position()} bytes read.")
          NoOp
        }
      }
    }

    ctx.pipeToSelf(result) {
      case Success(msg) => msg
      case Failure(t) => FutureFailed(new OstrostrojException(s"Next buffer failed!", t))
    }
  }

  def changeVolume(stepDelta: Int): Unit = {
    gainControl.setValue((((gainControl.getValue.toInt / volumeStepDb) + stepDelta)*volumeStepDb))
  }

  private def writeBuffer(buffer: ByteBuffer): Int = {
    val bytesWritten = sourceDataLine.write(buffer.array(), buffer.position(), buffer.limit())
    logger.trace(s"Bytes written = $bytesWritten")
    bytesWritten
  }
}

object PlaylistPlayerBehavior {
  private val logger = LoggerFactory.getLogger(classOf[PlaylistPlayerBehavior])
  private val volumeStepDb = 3
}