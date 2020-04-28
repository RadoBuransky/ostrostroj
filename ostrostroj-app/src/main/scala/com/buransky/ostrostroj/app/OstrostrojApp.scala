package com.buransky.ostrostroj.app

import akka.actor.typed.receptionist.Receptionist
import akka.actor.typed.scaladsl.{AbstractBehavior, ActorContext, Behaviors}
import akka.actor.typed.{ActorRef, Behavior, Terminated}
import akka.cluster.typed.Cluster
import com.buransky.ostrostroj.app.audio.AudioPlayer
import com.buransky.ostrostroj.app.audio.AudioPlayer.{MuteTrack, UnmuteTrack}
import com.buransky.ostrostroj.app.common.OstrostrojConfig.{DEV_DESKTOP, DEV_DEVICE}
import com.buransky.ostrostroj.app.common.{OstrostrojConfig, OstrostrojException}
import com.buransky.ostrostroj.app.controls.OstrostrojController
import com.buransky.ostrostroj.app.device.{DriverCommand, OdroidGpio}
import com.buransky.ostrostroj.app.show.PlaylistReader
import org.slf4j.LoggerFactory

object OstrostrojApp {
  private val logger = LoggerFactory.getLogger(OstrostrojApp.getClass)
  private val playlist = PlaylistReader.read(OstrostrojConfig.playlistPath)

  def apply(): Behavior[_] = Behaviors.setup[Receptionist.Listing] { ctx =>
    new OstrostrojApp(ctx)
  }

  class OstrostrojApp(ctx: ActorContext[Receptionist.Listing]) extends AbstractBehavior[Receptionist.Listing](ctx) {
    ctx.system.receptionist ! Receptionist.Subscribe(OdroidGpio.odroidGpioKey, ctx.self)

    initDesktopAndDeviceParts(ctx)

    override def onMessage(msg: Receptionist.Listing): Behavior[Receptionist.Listing] = {
      Behaviors.receive {
        case (ctx, OdroidGpio.odroidGpioKey.Listing(listings)) =>
          listings.foreach(initDriverDependencies(_, ctx))
          Behaviors.same
        case (_, Terminated(_)) => Behaviors.stopped
      }
    }

    private def initDesktopAndDeviceParts(ctx: ActorContext[_]): Unit = {
      if (OstrostrojConfig.develeoperMode) {
        val cluster = Cluster(ctx.system)
        if (cluster.selfMember.hasRole(DEV_DEVICE)) {
          initDevicePart(ctx)
        } else {
          if (cluster.selfMember.hasRole(DEV_DESKTOP)) {
            initDesktopPart(ctx)
          } else {
            throw new OstrostrojException(s"Invalid cluster role!")
          }
        }
      } else {
        initDevicePart(ctx)
        initDesktopPart(ctx)
      }
    }

    private def initDriverDependencies(driver: ActorRef[DriverCommand],
                                       ctx: ActorContext[_]): Unit = {
      logger.debug("OdroidC2Driver discovered by receptionist.")
      if (shouldSpawnController(ctx)) {
        ctx.watch(ctx.spawn(OstrostrojController(driver), "controller"))
      }
    }

    private def shouldSpawnController(ctx: ActorContext[_]): Boolean = {
      if (OstrostrojConfig.develeoperMode) {
        val cluster = Cluster(ctx.system)
        cluster.selfMember.hasRole(DEV_DESKTOP)
      }
      else {
        true
      }
    }

    private def initDevicePart(ctx: ActorContext[_]): Unit = {
      ctx.watch(ctx.spawn(OdroidGpio(), "gpio"))
    }

    private def initDesktopPart(ctx: ActorContext[_]): Unit = {
      import concurrent.duration._
      val audioPlayer = ctx.spawn(AudioPlayer(ctx.self), "audioPlayer")
      ctx.watch(audioPlayer)

      val tracks = playlist.songs.head.audio :: playlist.songs.head.tracks.map(_.audio).toList
      audioPlayer ! AudioPlayer.Load(tracks)
      audioPlayer ! AudioPlayer.Play

      // TODO: Just for testing, remove!
      ctx.scheduleOnce(3.seconds, audioPlayer, MuteTrack(1))
      ctx.scheduleOnce(5.seconds, audioPlayer, MuteTrack(2))
      ctx.scheduleOnce(6.seconds, audioPlayer, UnmuteTrack(1))
      ctx.scheduleOnce(7.seconds, audioPlayer, MuteTrack(3))
      ctx.scheduleOnce(8.seconds, audioPlayer, UnmuteTrack(2))
      ctx.scheduleOnce(9.seconds, audioPlayer, MuteTrack(4))
      ctx.scheduleOnce(10.seconds, audioPlayer, UnmuteTrack(3))
    }
  }
}
