package com.buransky.ostrostroj.app.controller

import akka.actor.typed.scaladsl.Behaviors
import akka.actor.typed.{ActorRef, Behavior}
import com.buransky.ostrostroj.app.controller.Max7219.Config
import com.buransky.ostrostroj.app.device.{DigitalPinState, Gpio, PinCommand}

object LedMatrix {
  sealed trait LedMatrixCommand
  case object ClearScreen extends LedMatrixCommand
  case object Test extends LedMatrixCommand

  def apply(driver: ActorRef[PinCommand], config: Config): Behavior[LedMatrixCommand] = Behaviors.setup { ctx =>
    val ledMatrix = ctx.spawn(Max7219(driver, Max7219.Config(Gpio.Pin3, Gpio.Pin4, Gpio.Pin5)), "max7219")

    Behaviors.receiveMessage {
      case ClearScreen =>
        ledMatrix ! Max7219.Command(din = DigitalPinState(false), cs = DigitalPinState(false), clk = DigitalPinState(false))
        Behaviors.same
      case Test =>
        Behaviors.same
    }
  }
}
