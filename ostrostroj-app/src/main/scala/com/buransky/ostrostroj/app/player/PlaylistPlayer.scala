package com.buransky.ostrostroj.app.player

import java.nio.ByteBuffer
import java.nio.file.Path
import java.time.{Duration, Instant}

import akka.actor.typed.scaladsl.{AbstractBehavior, ActorContext, Behaviors}
import akka.actor.typed.{ActorRef, Behavior, PostStop, Signal}
import com.buransky.ostrostroj.app.common.OstrostrojException
import com.buransky.ostrostroj.app.show.Playlist
import com.sun.media.sound.WaveFileReader
import javax.sound.sampled.{AudioSystem, Mixer, SourceDataLine}
import org.slf4j.LoggerFactory

import scala.concurrent.Future

object PlaylistPlayer {
  private val logger = LoggerFactory.getLogger(PlaylistPlayer.getClass)

  private val mixerName = "Digital Output" // Desktop mixer for testing
  //  private val mixerName = "ODROIDDAC" // Odroid HiFi Shield
  private val bufferDuration = Duration.ofMillis(50);

  final case class PlayerStatus(playlist: Playlist,
                                songIndex: Int,
                                songDuration: Duration,
                                songPosition: Instant,
                                loop: Option[(Instant, Instant)],
                                mutedTracks: Seq[Boolean],
                                isPlaying: Boolean)

  sealed trait Command
  final case object Play extends Command
  final case object Pause extends Command
  final case object StartLooping extends Command
  final case object StopLooping extends Command
  final case class UnmuteTrack(trackIndex: Int) extends Command
  final case class MuteTrack(trackIndex: Int) extends Command
  final case object NextSong extends Command
  final case object VolumeUp extends Command
  final case object VolumeDown extends Command
  final case class ReportStatusTo(actorRef: ActorRef[_]) extends Command

  def apply(playlist: Playlist): Behavior[Command] = Behaviors.setup { ctx =>
    val mixer = getMixer(mixerName)
    val songIndex = 0
    val firstSongFirstTrack = playlist.songs(songIndex).tracks.head.audio
    val sourceDataLine = getSourceDataLine(firstSongFirstTrack, mixer.getMixerInfo)
    val songStream = createSongStream(playlist, songIndex, sourceDataLine)
    new PlaylistPlayerBehavior(playlist, songIndex, songStream, mixer, sourceDataLine, ctx)
  }

  class PlaylistPlayerBehavior(playlist: Playlist,
                               currentSongIndex: Int,
                               currentSongStream: SongStream,
                               mixer: Mixer,
                               sourceDataLine: SourceDataLine,
                               ctx: ActorContext[Command]) extends AbstractBehavior[Command](ctx) {
    import ctx.executionContext

    playCurrentSong()

    private val nextSongStream: Option[SongStream] = if (currentSongIndex + 1 < playlist.songs.length) {
      Some(createSongStream(playlist, currentSongIndex + 1, sourceDataLine))
    } else {
      None
    }

    override def onMessage(msg: Command): Behavior[Command] = msg match {
      case Play =>
        sourceDataLine.start()
        Behaviors.same
      case Pause =>
        sourceDataLine.stop()
        Behaviors.same
      case StartLooping => Behaviors.same
      case StopLooping => Behaviors.same
      case UnmuteTrack(trackIndex) => Behaviors.same
      case MuteTrack(trackIndex) => Behaviors.same
      case NextSong =>
        nextSongStream match {
          case None => Behaviors.same
          case Some(next) => new PlaylistPlayerBehavior(playlist, currentSongIndex + 1, next, mixer, sourceDataLine,
            ctx)
        }
      case VolumeUp => Behaviors.same
      case VolumeDown => Behaviors.same
      case ReportStatusTo(_) => Behaviors.same
    }

    override def onSignal: PartialFunction[Signal, Behavior[Command]] = {
      case PostStop =>
        sourceDataLine.close()
        mixer.close()
        currentSongStream.close()
        nextSongStream.foreach(_.close())
        logger.info("Playlist player closed.")
        Behaviors.stopped
    }

    private def playCurrentSong(): Future[_] = {
      currentSongStream.byteBuffer.map { buffer =>
        val bytesRead = buffer.limit() - buffer.position()
        if (bytesRead == 0) {
          skipToNextSong()
        } else {
          val bytesWritten = writeAudioData(buffer)
          if (bytesWritten == bytesRead) {
            playCurrentSong()
          }
        }
      }(ctx.executionContext)
    }

    private def writeAudioData(buffer: ByteBuffer): Int = {
      val bytesWritten = sourceDataLine.write(buffer.array(), buffer.position(), buffer.limit() - buffer.position())
      logger.trace(s"Bytes written = $bytesWritten")
      bytesWritten
    }

    private def skipToNextSong(): Unit = {
      nextSongStream match {
        case None =>
          logger.info(s"Playlist playback finished.")
        case Some(_) =>
          logger.debug(s"Switching to the next song.")
          ctx.self ! NextSong
      }
    }
  }

  private def createSongStream(playlist: Playlist, songIndex: Int, sourceDataLine: SourceDataLine): SongStream =
    new SongStream(playlist.songs(songIndex).tracks.map(_.audio), sourceDataLine.getFormat)

  private def getSourceDataLine(firstSongFirstTrack: Path, mixerInfo: Mixer.Info): SourceDataLine = {
    val waveFileReader = new WaveFileReader()
    val format = waveFileReader.getAudioFileFormat(firstSongFirstTrack.toFile).getFormat
    val sourceDataLine = AudioSystem.getSourceDataLine(format, mixerInfo)

    val bufferSize = bufferDuration.toMillis*format.getSampleRate*(format.getSampleSizeInBits/8)*format.getChannels/1000
    sourceDataLine.open(format, bufferSize.toInt)
    logger.info(s"Source data line open. [${format.getSampleRate}, ${format.getSampleSizeInBits}, " +
      s"${format.getChannels}, buffer size = ${sourceDataLine.getBufferSize}]")
    sourceDataLine
  }

  private def getMixer(mixerName: String): Mixer = {
    val mixerInfo = getMixerInfo(mixerName)
    val mixer = AudioSystem.getMixer(mixerInfo)
    mixer.open()
    logger.info(s"Audio mixer is open. [${mixer.getMixerInfo.getName}]")
    mixer
  }

  private def getMixerInfo(mixerName: String): Mixer.Info = {
    AudioSystem.getMixerInfo.find(_.getName.contains(mixerName))
      .getOrElse(throw new OstrostrojException(s"Audio mixer for not found! [$mixerName]"))
  }
}