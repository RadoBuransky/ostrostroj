package com.buransky.ostrostroj.app.controller

import akka.actor.typed.scaladsl.{AbstractBehavior, ActorContext, Behaviors}
import akka.actor.typed.{ActorRef, Behavior}
import com.buransky.ostrostroj.app.controller.hw.part.RgbLed
import com.buransky.ostrostroj.app.controller.hw.part.RgbLed.Color
import com.buransky.ostrostroj.app.controller.hw.{DigitalPinState, Gpio, PinCommand}
import com.typesafe.config.Config

/**
 * Logical API for physical floor pedal controller (buttons, LEDs, ...).
 */
object PedalController {
  case class Params(useEmulator: Boolean)
  object Params {
    def apply(ostrostrojConfig: Config): Params = {
      Params(ostrostrojConfig.getBoolean("emulator"))
    }
  }

  sealed trait ControllerCommand
  final case class LedControllerCommand(led: Model.Led, ledCommand: Model.LedColor) extends ControllerCommand

  sealed trait ControllerEvent

  def apply(config: Params,
            emulatorFactory: () => Behavior[PinCommand],
            odroidFactory: () => Behavior[PinCommand],
            ledFactory: (ActorRef[PinCommand], RgbLed.Config) => Behavior[Color]): Behavior[ControllerCommand] =
    if (config.useEmulator) apply(emulatorFactory, ledFactory) else apply(odroidFactory, ledFactory)

  def apply(driverFactory: () => Behavior[PinCommand],
            ledFactory: (ActorRef[PinCommand], RgbLed.Config) => Behavior[Color]): Behavior[ControllerCommand] = Behaviors.setup { ctx =>
    new PedalControllerBehavior(driverFactory, ledFactory, ctx)
  }

  class PedalControllerBehavior(driverFactory: () => Behavior[PinCommand],
                                ledFactory: (ActorRef[PinCommand], RgbLed.Config) => Behavior[Color],
                                context: ActorContext[ControllerCommand]) extends AbstractBehavior[ControllerCommand](context) {
    private val driver = context.spawn(driverFactory(), "driver")
    private val led1 = context.spawn(ledFactory(driver, RgbLed.Config(Gpio.Pin0, Gpio.Pin1, Gpio.Pin2)), "led1")

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
