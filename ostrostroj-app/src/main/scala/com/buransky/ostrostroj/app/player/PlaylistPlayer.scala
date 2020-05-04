package com.buransky.ostrostroj.app.player

import java.nio.file.Path
import java.time.{Duration, Instant}

import akka.actor.typed.scaladsl.{AbstractBehavior, ActorContext, Behaviors}
import akka.actor.typed.{ActorRef, Behavior, PostStop, Signal}
import com.buransky.ostrostroj.app.common.OstrostrojException
import com.buransky.ostrostroj.app.show.{Playlist, Track}
import com.sun.media.sound.WaveFileReader
import javax.sound.sampled._
import org.slf4j.LoggerFactory

import scala.annotation.tailrec

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
    val firstSongFirstTrack = playlist.songs(songIndex).path
    val sourceDataLine = getSourceDataLine(firstSongFirstTrack, mixer.getMixerInfo)
    new PlaylistPlayerBehavior(playlist, songIndex, mixer, sourceDataLine, ctx)
  }

  class PlaylistPlayerBehavior(playlist: Playlist,
                               songIndex: Int,
                               mixer: Mixer,
                               sourceDataLine: SourceDataLine,
                               ctx: ActorContext[Command]) extends AbstractBehavior[Command](ctx) {
    logger.debug(s"Playlist player created. [$songIndex]")
    private val songStream: MainStream = createSongStream(playlist, songIndex)
    private val buffer: Array[Byte] = new Array[Byte](sourceDataLine.getBufferSize)
    playSongStream()

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
      case NextSong if songIndex == playlist.songs.length - 1 =>
        logger.info(s"This is the last song. Can't proceed to the next one. [$songIndex]")
        Behaviors.same
      case NextSong => new PlaylistPlayerBehavior(playlist, songIndex + 1, mixer, sourceDataLine, ctx)
      case VolumeUp => Behaviors.same
      case VolumeDown => Behaviors.same
      case ReportStatusTo(_) => Behaviors.same
    }

    override def onSignal: PartialFunction[Signal, Behavior[Command]] = {
      case PostStop =>
        sourceDataLine.close()
        mixer.close()
        songStream.close()
        logger.info("Playlist player closed.")
        Behaviors.stopped
    }

    @tailrec
    private def playSongStream(): Unit = {
      val bytesRead = songStream.read(buffer)
      logger.trace(s"Bytes read = $bytesRead")
      if (bytesRead < 1) {
        logger.debug(s"Playback of song finished. Automatically proceeding to the next song. [$songIndex]")
        ctx.self ! NextSong
      } else {
        val bytesWritten = writeBuffer(buffer, bytesRead)
        if (bytesWritten == bytesRead) {
          playSongStream()
        } else {
          logger.debug(s"$bytesWritten bytes written but $bytesRead bytes read.")
        }
      }
    }

    private def writeBuffer(buffer: Array[Byte], bytesRead: Int): Int = {
      val bytesWritten = sourceDataLine.write(buffer, 0, bytesRead)
      logger.trace(s"Bytes written = $bytesWritten")
      bytesWritten
    }

    private def createSongStream(playlist: Playlist, songIndex: Int): MainStream = {
      val song = playlist.songs(songIndex)
//      val audioInputStreams = loadAudioInputStreams(song.tracks)
      new MainStream(song)
    }

    private def loadAudioInputStreams(tracks: Seq[Track]): Seq[AudioInputStream] = {
      tracks.map { track =>
        val audioInputStream = AudioSystem.getAudioInputStream(track.path.toFile)
        if (!checkAudioFormat(sourceDataLine.getFormat, audioInputStream.getFormat)) {
          throw new OstrostrojException(s"Not the same audio format! [${track.path}]")
        }
        audioInputStream
      }
    }

    private def checkAudioFormat(expected: AudioFormat, actual: AudioFormat): Boolean = {
      expected.getSampleRate != actual.getSampleRate || expected.getSampleSizeInBits != actual.getSampleSizeInBits ||
      expected.getChannels != actual.getChannels || expected.getEncoding != actual.getEncoding ||
      expected.isBigEndian != actual.isBigEndian
    }
  }

  private def getSourceDataLine(firstSongMaster: Path, mixerInfo: Mixer.Info): SourceDataLine = {
    val waveFileReader = new WaveFileReader()
    val format = waveFileReader.getAudioFileFormat(firstSongMaster.toFile).getFormat
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