package com.buransky.ostrostroj.app

import akka.NotUsed
import akka.actor.typed.scaladsl.Behaviors
import akka.actor.typed.{ActorSystem, Behavior, Terminated}
import com.buransky.ostrostroj.app.audio.AudioPlayer
import com.buransky.ostrostroj.app.controller.PedalController
import com.buransky.ostrostroj.app.controller.PedalController.ControllerCommand
import com.buransky.ostrostroj.app.controller.hw.part.RgbLed
import com.buransky.ostrostroj.app.controller.hw.{EmulatorDriver, OdroidC2Driver}
import com.buransky.ostrostroj.app.show.PerformanceManager
import com.typesafe.config.ConfigFactory
import org.slf4j.LoggerFactory

/**
 * Ostrostroj App entry point.
 */
object Main {
  private val config = ConfigFactory.load()
  private val ostrostrojConfig = config.getConfig("ostrostroj")
  private val logger = LoggerFactory.getLogger(this.getClass)

  def main(args: Array[String]): Unit = {
    initLogging()
    ActorSystem(Main(), "ostrostroj", config)
  }

  def apply(): Behavior[NotUsed] = Behaviors.setup { context =>
    val performanceManager = context.spawn(PerformanceManager(), "performanceManager")
    val controller = context.spawn(pedalControllerBehavior(), "controller")
    val audioPlayer = context.spawn(AudioPlayer(), "audioPlayer")

    Behaviors.receiveSignal {
      case (_, Terminated(_)) => Behaviors.stopped
    }
  }

  private def pedalControllerBehavior(): Behavior[ControllerCommand] = {
    PedalController(PedalController.Params(ostrostrojConfig), () => EmulatorDriver(), () => OdroidC2Driver(),
      (driver, config) => RgbLed(driver, config))
  }

  private def initLogging(): Unit = {
    import org.slf4j.bridge.SLF4JBridgeHandler
    SLF4JBridgeHandler.removeHandlersForRootLogger()
    SLF4JBridgeHandler.install()
  }
}
