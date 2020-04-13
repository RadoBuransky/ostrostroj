package com.buransky.ostrostroj.app.controller

import akka.actor.typed.scaladsl.{AbstractBehavior, ActorContext, Behaviors}
import akka.actor.typed.{ActorRef, Behavior}
import com.buransky.ostrostroj.app.controller.hw.OdroidC2Driver.Gpio
import com.buransky.ostrostroj.app.controller.hw.OdroidC2Driver.Gpio.PinState
import com.buransky.ostrostroj.app.controller.hw.part.RgbLed
import com.buransky.ostrostroj.app.controller.hw.part.RgbLed.Config

/**
 * Logical API for physical floor pedal controller (buttons, LEDs, ...).
 */
object PedalController {
  sealed trait ControllerCommand
  final case class LedControllerCommand(led: Model.Led, ledCommand: Model.LedColor) extends ControllerCommand

  sealed trait ControllerEvent

  def apply(): Behavior[ControllerCommand] = Behaviors.setup { context =>
    new PedalControllerBehavior(context)
  }

  class PedalControllerBehavior(context: ActorContext[ControllerCommand]) extends AbstractBehavior[ControllerCommand](context) {
    private val led1 = context.spawn(RgbLed(Config(Gpio.Pin1, Gpio.Pin2, Gpio.Pin3)), "led1")

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
      RgbLed.Color(PinState(ledColor.r), PinState(ledColor.g), PinState(ledColor.b))
  }
}
