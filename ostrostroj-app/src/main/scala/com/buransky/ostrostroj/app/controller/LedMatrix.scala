package com.buransky.ostrostroj.app.controller

import akka.actor.typed.scaladsl.Behaviors
import akka.actor.typed.{ActorRef, Behavior}
import com.buransky.ostrostroj.app.controller.Max7219.Config
import com.buransky.ostrostroj.app.device.PinCommand

object LedMatrix {
  sealed trait LedMatrixCommand
  case class Max7219Word(max7219: Max7219.Word) extends LedMatrixCommand
  case object ClearScreen extends LedMatrixCommand
  case object Test extends LedMatrixCommand

  def apply(driver: ActorRef[PinCommand], config: Config): Behavior[LedMatrixCommand] = Behaviors.setup { ctx =>
    val max7219 = ctx.spawn(Max7219(driver, config), "max7219")

    Behaviors.receiveMessage {
      case ClearScreen =>
        Behaviors.same
      case Test =>
        // Turn off and then on
        max7219 ! Max7219.Word(0x0C, 0x00)
        max7219 ! Max7219.Word(0x0C, 0x01)

        // Turn off test
        max7219 ! Max7219.Word(0x0F, 0x00)
        // Set decode mode
        max7219 ! Max7219.Word(0x09, 0x00)
        // Set scan limit
        max7219 ! Max7219.Word(0x0B, 0x07)

        // Show some data
        max7219 ! Max7219.Word(0x01, 0x01)
        max7219 ! Max7219.Word(0x02, 0x02)
        max7219 ! Max7219.Word(0x03, 0x04)
        max7219 ! Max7219.Word(0x04, 0x08)
        max7219 ! Max7219.Word(0x05, 0x10)
        max7219 ! Max7219.Word(0x06, 0x20)
        max7219 ! Max7219.Word(0x07, 0x40)
        max7219 ! Max7219.Word(0x08, 0x80.toByte)
        Behaviors.same

      case Max7219Word(word) =>
        max7219 ! word
        Behaviors.same
    }
  }
}
