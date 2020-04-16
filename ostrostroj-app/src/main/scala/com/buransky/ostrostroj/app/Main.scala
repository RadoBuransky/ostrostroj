package com.buransky.ostrostroj.app

import akka.actor.typed.receptionist.Receptionist
import akka.actor.typed.scaladsl.{ActorContext, Behaviors}
import akka.actor.typed.{ActorRef, ActorSystem, Behavior, Terminated}
import akka.cluster.typed.Cluster
import com.buransky.ostrostroj.app.audio.AudioPlayer
import com.buransky.ostrostroj.app.common.OstrostrojConfig._
import com.buransky.ostrostroj.app.common.{OstrostrojConfig, OstrostrojException}
import com.buransky.ostrostroj.app.controller.PedalController
import com.buransky.ostrostroj.app.device.{DriverCommand, OdroidC2Driver}
import com.buransky.ostrostroj.app.show.PerformanceManager
import org.slf4j.LoggerFactory

/**
 * Ostrostroj App entry point.
 */
object Main {
  private val logger = LoggerFactory.getLogger(this.getClass)

  def main(args: Array[String]): Unit = {
    initLogging()
    if (OstrostrojConfig.develeoperMode) {
      logger.warn("Ostrostroj running in developer mode using Akka cluster.")
    } else {
      logger.info(s"Ostrostroj running in production mode.")
    }

    ActorSystem(Main(), ACTOR_SYSTEM_NAME, config)
  }

  def apply(): Behavior[_] = Behaviors.setup[Receptionist.Listing] { ctx =>
    initDesktopAndDeviceParts(ctx)
    driverDiscovery(ctx)
  }

  private def driverDiscovery(ctx: ActorContext[Receptionist.Listing]): Behavior[Receptionist.Listing] = {
    ctx.system.receptionist ! Receptionist.Subscribe(OdroidC2Driver.odroidC2DriverKey, ctx.self)
    Behaviors.receive {
      case (ctx, OdroidC2Driver.odroidC2DriverKey.Listing(listings)) =>
        logger.debug("OdroidC2Driver discovered by receptionist.")
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
    ctx.spawn(PedalController(driver), "controller")
  }

  private def initDevicePart(ctx: ActorContext[_]): Unit = {
    ctx.spawn(OdroidC2Driver(), "driver")
  }

  private def initDesktopPart(ctx: ActorContext[_]): Unit = {
    ctx.spawn(PerformanceManager(), "performanceManager")
    ctx.spawn(AudioPlayer(), "audioPlayer")
  }

  private def initLogging(): Unit = {
    import org.slf4j.bridge.SLF4JBridgeHandler
    SLF4JBridgeHandler.removeHandlersForRootLogger()
    SLF4JBridgeHandler.install()
  }
}
