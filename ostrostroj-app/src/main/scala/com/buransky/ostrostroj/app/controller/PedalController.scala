package com.buransky.ostrostroj.app.controller

import akka.actor.typed.scaladsl.{AbstractBehavior, ActorContext, Behaviors}
import akka.actor.typed.{ActorRef, Behavior}
import com.buransky.ostrostroj.app.controller.hw.part.RgbLed
import com.buransky.ostrostroj.app.controller.hw.part.RgbLed.Config
import com.buransky.ostrostroj.app.controller.hw.{DigitalPinState, Gpio, OdroidC2Driver}

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
    private val driver = context.spawn(OdroidC2Driver(), "odroid")
    private val led1 = context.spawn(RgbLed(driver, Config(Gpio.Pin0, Gpio.Pin1, Gpio.Pin2)), "led1")

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
      RgbLed.Color(DigitalPinState(ledColor.r), DigitalPinState(ledColor.g), DigitalPinState(ledColor.b))
  }
}
