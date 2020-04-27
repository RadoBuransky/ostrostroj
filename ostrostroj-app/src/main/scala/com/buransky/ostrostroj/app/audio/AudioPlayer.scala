package com.buransky.ostrostroj.app.audio

import java.nio.file.Path

import akka.actor.typed.scaladsl.{AbstractBehavior, ActorContext, Behaviors}
import akka.actor.typed.{ActorRef, Behavior, PostStop, Signal}
import com.buransky.ostrostroj.app.common.OstrostrojException
import javax.sound.sampled.{AudioSystem, Mixer}
import org.slf4j.LoggerFactory

/**
 * Multi-track audio player capable of looping, track un/muting and software down-mixing.
 */
object AudioPlayer {
  private val HIFI_SHIELD_2 = "ODROIDDAC"
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
      try {
        AudioSystem.getMixerInfo.foreach { mixerInfo =>
          logger.debug(s"Name: ${mixerInfo.getName}")
          logger.debug(s"Vendor: ${mixerInfo.getVendor}")
          logger.debug(s"Description: ${mixerInfo.getDescription}")

          val mixer = AudioSystem.getMixer(mixerInfo)
          val sourceLinesInfo = mixer.getSourceLineInfo.map(_.toString)
            .mkString("Source lines:" + System.lineSeparator(), System.lineSeparator(), "")
          logger.debug(sourceLinesInfo)
        }
      }
      catch {
        case _: Throwable => // Ignore intentionally
      }
    }

    private val mixer: Mixer = AudioSystem.getMixer(getHifiShieldMixerInfo)
    logger.info("Audio mixer open.")

    private def getHifiShieldMixerInfo: Mixer.Info = {
      AudioSystem.getMixerInfo.find(_.getName.contains(HIFI_SHIELD_2))
        .getOrElse(throw new OstrostrojException("Audio mixer for HiFi shield not found!"))
    }

    override def onMessage(msg: Command): Behavior[Command] = Behaviors.ignore

    override def onSignal: PartialFunction[Signal, Behavior[Command]] = {
      case PostStop =>
        mixer.close()
        logger.info("Audio mixer closed.")
        Behaviors.same
    }
  }
}
