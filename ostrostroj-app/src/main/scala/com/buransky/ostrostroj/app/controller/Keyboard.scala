package com.buransky.ostrostroj.app.controller

import akka.actor.typed.scaladsl.Behaviors
import akka.actor.typed.{ActorRef, Behavior}
import com.buransky.ostrostroj.app.controller.LedMatrix.Max7219Word
import com.buransky.ostrostroj.app.device.Gpio.{Pin0, Pin1, Pin2}
import com.buransky.ostrostroj.app.device.{DigitalPinState, PinCommand}
import org.slf4j.LoggerFactory

import scala.concurrent.Future
import scala.util.{Failure, Success}

object Keyboard {
  private val logger = LoggerFactory.getLogger(Keyboard.getClass)

  final case class ListenForKey()

  def apply(driver: ActorRef[PinCommand],
            ledMatrix: ActorRef[LedMatrix.LedMatrixCommand]): Behavior[ListenForKey] = Behaviors.setup { ctx =>
    ctx.self ! ListenForKey()
    Behaviors.receiveMessage {
      case ListenForKey() =>
        val futureLine = Future(Console.in.readLine())(ctx.executionContext)
        ctx.pipeToSelf(futureLine) {
          case Success(line) =>
            line match {
              case "R" => driver ! PinCommand(Pin2, DigitalPinState(high = true))
              case "G" => driver ! PinCommand(Pin1, DigitalPinState(high = true))
              case "B" => driver ! PinCommand(Pin0, DigitalPinState(high = true))

              case "r" => driver ! PinCommand(Pin2, DigitalPinState(high = false))
              case "g" => driver ! PinCommand(Pin1, DigitalPinState(high = false))
              case "b" => driver ! PinCommand(Pin0, DigitalPinState(high = false))
              case _ =>
                val pair = line.split('-').map(s => Integer.parseInt(s).toByte)
                ledMatrix ! Max7219Word(Max7219.Word(pair(0), pair(1)))
            }

            ListenForKey()
          case Failure(e) =>
            logger.error(s"Key read error!", e)
            ListenForKey()
        }
        Behaviors.same
    }
  }
}
