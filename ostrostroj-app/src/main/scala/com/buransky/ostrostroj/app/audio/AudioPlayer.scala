package com.buransky.ostrostroj.app.audio

import java.nio.file.Path

import akka.actor.typed.scaladsl.{AbstractBehavior, ActorContext, Behaviors}
import akka.actor.typed.{ActorRef, Behavior}
import javax.sound.sampled.{AudioSystem, SourceDataLine}
import org.slf4j.LoggerFactory

/**
 * Multi-track audio player capable of looping, track un/muting and software down-mixing.
 */
object AudioPlayer {
  private val logger = LoggerFactory.getLogger(AudioPlayer.getClass)

  final case class Position(sample: Int)

  sealed trait Event
  final case class PositionUpdated(current: Position) extends Event
  final case object PlaybackFinished

  sealed trait Command
  final case class Load(tracks: Seq[Path]) extends Command
  final case object Play extends Command
  final case object Pause extends Command
  final case class StartLooping(start: Position, end: Position) extends Command
  final case object StopLooping extends Command
  final case class UnmuteTrack(trackIndex: Int) extends Command
  final case class MuteTrack(trackIndex: Int) extends Command

  def apply(listener: ActorRef[_]): Behavior[Command] = Behaviors.setup { ctx =>
    new AudioPlayerBehavior(listener, ctx)
  }

  class AudioPlayerBehavior(listener: ActorRef[_], ctx: ActorContext[Command])
    extends AbstractBehavior[Command](ctx) {

    if (logger.isDebugEnabled) {
      AudioSystem.getMixerInfo.foreach { mixerInfo =>
        logger.debug(s"${mixerInfo.getName}, ${mixerInfo.getVendor}, ${mixerInfo.getDescription}")
        val mixer = AudioSystem.getMixer(mixerInfo)

        val targetLinesInfo = mixer.getTargetLineInfo.map(_.toString)
          .mkString("Target lines:" + System.lineSeparator(), System.lineSeparator(), "")
        logger.debug(targetLinesInfo)

        val sourceLinesInfo = mixer.getSourceLineInfo.map(_.toString)
          .mkString("Source lines:" + System.lineSeparator(), System.lineSeparator(), "")
        logger.debug(sourceLinesInfo)
      }
    }

    override def onMessage(msg: Command): Behavior[Command] = Behaviors.ignore
  }
}
