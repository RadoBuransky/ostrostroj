package com.buransky.ostrostroj.app.controller

import akka.actor.typed.scaladsl.{AbstractBehavior, ActorContext, Behaviors}
import akka.actor.typed.{ActorRef, Behavior}
import com.buransky.ostrostroj.app.common.OstrostrojConfig
import com.buransky.ostrostroj.app.device._

/**
 * Logical API for physical floor pedal controller (buttons, LEDs, ...).
 */
object PedalController {
  sealed trait ControllerCommand
  final case class LedControllerCommand(led: Model.Led, ledCommand: Model.LedColor) extends ControllerCommand

  sealed trait ControllerEvent

  def apply(driver: ActorRef[DriverCommand]): Behavior[ControllerCommand] = Behaviors.setup { ctx =>
    new PedalControllerBehavior(driver, ctx)
  }

  class PedalControllerBehavior(driver: ActorRef[DriverCommand],
                                ctx: ActorContext[ControllerCommand]) extends AbstractBehavior[ControllerCommand](ctx) {
    private val ledMatrix = ctx.spawn(LedMatrix(driver,
      OldMax7219.Config(dinPin = Pin5, csPin = Pin4, clkPin = Pin3)), "ledMatrix")
    private val led1 = ctx.spawn(RgbLed(driver, RgbLed.Config(Pin0, Pin1, Pin2)), "led1")

    if (OstrostrojConfig.develeoperMode) {
      ctx.spawn(Keyboard(driver, ledMatrix), "keyboard")
    }

    ledMatrix ! LedMatrix.Test

    override def onMessage(message: ControllerCommand): Behavior[ControllerCommand] = {
      message match {
        case LedControllerCommand(led, ledColor) =>
          // Translate logical command to physical
          hwLedActorRef(led) ! hwLedCommand(ledColor)
          Behaviors.same
      }
    }

    /**
     * Maps logical LED to physical LED.
     */
    private def hwLedActorRef(led: Model.Led): ActorRef[RgbLed.Color] = {
      led match {
        case Model.LedBtn1.led => led1
      }
    }

    /**
     * Maps logical LED command to physical LED command.
     */
    private def hwLedCommand(ledColor: Model.LedColor): RgbLed.Color =
      RgbLed.Color(ledColor.r, ledColor.g, ledColor.b)
  }
}
