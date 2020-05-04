package com.buransky.ostrostroj.app.player

import java.nio.file.Path
import java.time.{Duration, Instant}

import akka.actor.typed.scaladsl.{AbstractBehavior, ActorContext, Behaviors}
import akka.actor.typed.{ActorRef, Behavior, PostStop, Signal}
import com.buransky.ostrostroj.app.common.OstrostrojException
import com.buransky.ostrostroj.app.show.Playlist
import com.sun.media.sound.WaveFileReader
import javax.sound.sampled._
import org.slf4j.LoggerFactory

import scala.concurrent.Future
import scala.util.{Failure, Success}

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
  final case object Harder extends Command
  final case object Softer extends Command
  final case object NextSong extends Command
  final case object VolumeUp extends Command
  final case object VolumeDown extends Command
  final case class ReportStatusTo(actorRef: ActorRef[_]) extends Command
  private final case object NextBuffer extends Command
  private final case object NoOp extends Command
  private final case class ChangeBehavior(newBehavior: PlaylistPlayerBehavior) extends Command
  private final case class FutureFailed(cause: Throwable) extends Command

  def apply(playlist: Playlist): Behavior[Command] = Behaviors.setup { ctx =>
    val mixer = getMixer(mixerName)
    val songIndex = 0
    val firstSongMasterTrack = playlist.songs(songIndex).path
    val sourceDataLine = getSourceDataLine(firstSongMasterTrack, mixer.getMixerInfo)
    new PlaylistPlayerBehavior(playlist, songIndex, mixer, sourceDataLine, ctx)
  }

  class PlaylistPlayerBehavior private (playlist: Playlist,
                               songIndex: Int,
                               looper: Looper,
                               mixer: Mixer,
                               sourceDataLine: SourceDataLine,
                               ctx: ActorContext[Command]) extends AbstractBehavior[Command](ctx) {
    import ctx.executionContext

    def this(playlist: Playlist, songIndex: Int, mixer: Mixer, sourceDataLine: SourceDataLine,
             ctx: ActorContext[Command]) {
      this(playlist, songIndex, new Looper(playlist.songs(songIndex)), mixer, sourceDataLine, ctx)
    }

    logger.debug(s"Playlist player created. [$songIndex]")
    private val masterStream: AudioInputStream = createMasterStream(playlist, songIndex)
    private var masterStreamPosition: Int = 0
    private val buffer: Array[Byte] = new Array[Byte](sourceDataLine.getBufferSize)

    override def onMessage(msg: Command): Behavior[Command] = msg match {
      case Play =>
        sourceDataLine.start()
        Behaviors.same
      case Pause =>
        sourceDataLine.stop()
        Behaviors.same
      case StartLooping =>
        startLooping()
        Behaviors.same
      case StopLooping => copy(looper.stopLooping())
      case Harder => copy(looper.harder())
      case Softer => copy(looper.softer())
      case NextSong if songIndex == playlist.songs.length - 1 => Behaviors.same
      case NextSong => copy(songIndex + 1)
      case VolumeUp => Behaviors.same
      case VolumeDown => Behaviors.same
      case ReportStatusTo(_) => Behaviors.same
      case NextBuffer =>
        nextBuffer()
        Behaviors.same
      case FutureFailed(t) => throw t
      case NoOp => Behaviors.same
    }

    private def startLooping(): Unit = {
      ctx.pipeToSelf(Future {
        copy(looper.startLooping(masterStreamPosition))
      }) {
        case Success(newBehavior) => ChangeBehavior(newBehavior)
        case Failure(t) => FutureFailed(new OstrostrojException(s"Start looping failed!", t))
      }
    }

    override def onSignal: PartialFunction[Signal, Behavior[Command]] = {
      case PostStop =>
        sourceDataLine.close()
        mixer.close()
        masterStream.close()
        logger.info("Playlist player closed.")
        Behaviors.stopped
    }

    private def nextBuffer(): Unit = {
      val result = Future {
        val bytesRead = read(buffer)
        if (bytesRead < 1) {
          logger.debug(s"Playback of song finished. Automatically proceeding to the next song. [$songIndex]")
          NextSong
        } else {
          val bytesWritten = writeBuffer(buffer, bytesRead)
          if (bytesWritten == bytesRead) {
            NextBuffer
          } else {
            logger.debug(s"$bytesWritten bytes written but $bytesRead bytes read.")
            NoOp
          }
        }
      }

      ctx.pipeToSelf(result) {
        case Success(msg) => msg
        case Failure(t) => FutureFailed(new OstrostrojException(s"Next buffer failed!", t))
      }
    }

    private def read(b: Array[Byte]): Int = {
      val looperReadResult = looper.read(b, masterStreamPosition)
      logger.trace(s"Looper read result = $looperReadResult")

      if (looperReadResult.masterSkip > 0) {
        masterStreamPosition += masterStream.skip(looperReadResult.masterSkip).toInt
      }

      if (looperReadResult.bytesRead > 0) {
        if (looperReadResult.bytesRead < b.length) {
          readFromMaster(b, looperReadResult.bytesRead) + looperReadResult.bytesRead
        } else {
          looperReadResult.bytesRead
        }
      } else {
        readFromMaster(b, 0)
      }
    }

    private def readFromMaster(b: Array[Byte], offset: Int): Int = {
      val bytesRead = masterStream.read(b, offset, b.length - offset)
      logger.trace(s"Bytes read from master = $bytesRead")
      if (bytesRead > 0) {
        masterStreamPosition += bytesRead
      }
      bytesRead
    }

    private def writeBuffer(buffer: Array[Byte], bytesRead: Int): Int = {
      val bytesWritten = sourceDataLine.write(buffer, 0, bytesRead)
      logger.trace(s"Bytes written = $bytesWritten")
      bytesWritten
    }

    private def createMasterStream(playlist: Playlist, songIndex: Int): AudioInputStream = {
      val song = playlist.songs(songIndex)
      val audioInputStream = AudioSystem.getAudioInputStream(song.path.toFile)
      if (!checkAudioFormat(sourceDataLine.getFormat, audioInputStream.getFormat)) {
        throw new OstrostrojException(s"Not the same audio format! [${song.path}]")
      }
      audioInputStream
    }

    private def checkAudioFormat(expected: AudioFormat, actual: AudioFormat): Boolean = {
      expected.getSampleRate != actual.getSampleRate || expected.getSampleSizeInBits != actual.getSampleSizeInBits ||
      expected.getChannels != actual.getChannels || expected.getEncoding != actual.getEncoding ||
      expected.isBigEndian != actual.isBigEndian
    }

    private def copy(songIndex: Int): PlaylistPlayerBehavior = {
      new PlaylistPlayerBehavior(playlist, songIndex, mixer, sourceDataLine, ctx)
    }

    private def copy(looper: Looper): PlaylistPlayerBehavior = {
      new PlaylistPlayerBehavior(playlist, songIndex, looper, mixer, sourceDataLine, ctx)
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