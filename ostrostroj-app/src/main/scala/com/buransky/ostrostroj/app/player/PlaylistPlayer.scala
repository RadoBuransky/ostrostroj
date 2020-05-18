package com.buransky.ostrostroj.app.player

import java.nio.file.Path
import java.time.Duration

import akka.actor.typed.scaladsl.Behaviors
import akka.actor.typed.{ActorRef, Behavior}
import com.buransky.ostrostroj.app.common.OstrostrojException
import com.buransky.ostrostroj.app.show.Playlist
import com.sun.media.sound.WaveFileReader
import javax.sound.sampled._
import org.slf4j.LoggerFactory

object PlaylistPlayer {
  private val logger = LoggerFactory.getLogger(PlaylistPlayer.getClass)

  private val mixerName = "Digital Output" // Desktop mixer for testing
  //  private val mixerName = "ODROIDDAC" // Odroid HiFi Shield
  private val bufferDuration = Duration.ofMillis(50);

  final case class LoopStatus(start: Duration,
                              end: Duration,
                              minLevel: Int,
                              maxLevel: Int,
                              currentLevel: Int,
                              targetLevel: Int,
                              isDraining: Boolean)
  final case class PlayerStatus(playlist: Playlist,
                                songIndex: Int,
                                songDuration: Duration,
                                songPosition: Duration,
                                loop: Option[LoopStatus],
                                isPlaying: Boolean,
                                masterGainDb: Double)

  sealed trait Command
  final case object Play extends Command
  final case object Pause extends Command
  final case object StartLooping extends Command
  final case object StopLooping extends Command
  final case object Harder extends Command
  final case object Softer extends Command
  final case class AutoplayNext(songIndex: Int) extends Command
  final case object VolumeUp extends Command
  final case object VolumeDown extends Command
  final case class GetStatus(replyTo: ActorRef[AnyRef]) extends Command
  private[player] final case object ReadNextBuffer extends Command
  private[player] final case object NoOp extends Command
  private[player] final case class FutureFailed(cause: Throwable) extends Command

  def apply(playlist: Playlist): Behavior[Command] = Behaviors.setup { ctx =>
    val mixer = getMixer(mixerName)
    val songIndex = 0
    val firstSongMasterTrack = playlist.songs(songIndex).path
    val sourceDataLine = getSourceDataLine(firstSongMasterTrack, mixer.getMixerInfo)
    val gainControl = sourceDataLine.getControl(FloatControl.Type.MASTER_GAIN) match {
      case fc: FloatControl => fc
      case other => throw new OstrostrojException(s"Master gain is not a FloatControl! [${other.getClass}]")
    }
    new PlaylistPlayerBehavior(playlist, songIndex, mixer, sourceDataLine, gainControl, ctx)
  }

  def framePositionToDuration(framePosition: Int, sampleRate: Double): Duration =
    Duration.ofMillis((1000.0*framePosition.toDouble/sampleRate).toInt)

  private def getSourceDataLine(firstSongMaster: Path, mixerInfo: Mixer.Info): SourceDataLine = {
    val waveFileReader = new WaveFileReader()
    val audioFileFormat = waveFileReader.getAudioFileFormat(firstSongMaster.toFile)
    val audioFormat = audioFileFormat.getFormat
    val sourceDataLine = AudioSystem.getSourceDataLine(audioFormat, mixerInfo)

    val bufferSize = bufferDuration.toMillis*audioFormat.getSampleRate*(audioFormat.getSampleSizeInBits/8)*audioFormat.getChannels/1000
    sourceDataLine.open(audioFormat, bufferSize.toInt)
    logger.info(s"Source data line open. [${audioFormat.getSampleRate}, ${audioFormat.getSampleSizeInBits}, " +
      s"${audioFormat.getChannels}, buffer size = ${sourceDataLine.getBufferSize}]")
    sourceDataLine
  }

  private def getMixer(mixerName: String): Mixer = {
    val mixerInfo = AudioSystem.getMixerInfo.find(_.getName.contains(mixerName))
      .getOrElse(throw new OstrostrojException(s"Audio mixer for not found! [$mixerName]"))
    val mixer = AudioSystem.getMixer(mixerInfo)
    mixer.open()
    logger.info(s"Audio mixer is open. [${mixer.getMixerInfo.getName}]")
    mixer
  }
}