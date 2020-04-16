package com.buransky.ostrostroj.app.controller

import akka.actor.typed.scaladsl.Behaviors
import akka.actor.typed.{ActorRef, Behavior}
import com.buransky.ostrostroj.app.controller.Max7219.Config
import com.buransky.ostrostroj.app.device.{DriverCommand, Word}

object LedMatrix {
  sealed trait LedMatrixCommand
  case class Max7219Word(max7219: Word) extends LedMatrixCommand
  case object ClearScreen extends LedMatrixCommand
  case object Test extends LedMatrixCommand

  def apply(driver: ActorRef[DriverCommand], config: Config): Behavior[LedMatrixCommand] = Behaviors.setup { ctx =>
//    val max7219 = ctx.spawn(Max7219(driver, config), "max7219")

    Behaviors.receiveMessage {
      case ClearScreen =>
        Behaviors.same
      case Test =>
        // Turn off and then on
        sendToAllChips(driver, 0x0C, 0x00)
        sendToAllChips(driver, 0x0C, 0x01)

        // Turn off test mode
        sendToAllChips(driver, 0x0F, 0x01)
        sendToAllChips(driver, 0x0F, 0x00)
        // Set decode mode (no decoding)
        sendToAllChips(driver, 0x09, 0x00)
        // Set scan limit (8 segments)
        sendToAllChips(driver, 0x0B, 0x07)
        // Set min intensity
        sendToAllChips(driver, 0x0A, 0x00)

        // Show some data
        drawSlash(driver, 0)

//        max7219 ! Max7219.Word(0x04, 0xFF.toByte, 1)
//
//        max7219 ! Max7219.Word(0x01, 0x80.toByte, 2)
//        max7219 ! Max7219.Word(0x02, 0x40, 2)
//        max7219 ! Max7219.Word(0x03, 0x20, 2)
//        max7219 ! Max7219.Word(0x04, 0x10, 2)
//        max7219 ! Max7219.Word(0x05, 0x08, 2)
//        max7219 ! Max7219.Word(0x06, 0x04, 2)
//        max7219 ! Max7219.Word(0x07, 0x02, 2)
//        max7219 ! Max7219.Word(0x08, 0x01, 2)
//
//        max7219 ! Max7219.Word(0x01, 0x04, 3)
//        max7219 ! Max7219.Word(0x02, 0x04, 3)
//        max7219 ! Max7219.Word(0x03, 0x04, 3)
//        max7219 ! Max7219.Word(0x04, 0x04, 3)
//        max7219 ! Max7219.Word(0x05, 0x04, 3)
//        max7219 ! Max7219.Word(0x06, 0x04, 3)
//        max7219 ! Max7219.Word(0x07, 0x04, 3)
//        max7219 ! Max7219.Word(0x08, 0x04, 3)
        Behaviors.same

      case Max7219Word(word) =>
//        max7219 ! word
        Behaviors.same
    }
  }

  private def drawSlash(driver: ActorRef[DriverCommand], chipIndex: Int): Unit = {
    driver ! Word(0x01, 0x01, chipIndex)
    driver ! Word(0x02, 0x02, chipIndex)
    driver ! Word(0x03, 0x04, chipIndex)
    driver ! Word(0x04, 0x08, chipIndex)
    driver ! Word(0x05, 0x10, chipIndex)
    driver ! Word(0x06, 0x20, chipIndex)
    driver ! Word(0x07, 0x40, chipIndex)
    driver ! Word(0x08, 0x80.toByte, chipIndex)
  }

  private def sendToAllChips(driver: ActorRef[DriverCommand], address: Byte, data: Byte): Unit = {
    for (i <- 0 to 3) {
      driver ! Word(address, data, i)
    }
  }
}
