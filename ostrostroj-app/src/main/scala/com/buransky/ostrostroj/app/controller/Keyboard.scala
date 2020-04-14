package com.buransky.ostrostroj.app.controller

import akka.actor.typed.{ActorRef, Behavior}
import akka.actor.typed.scaladsl.Behaviors
import com.buransky.ostrostroj.app.device.Gpio.{Pin0, Pin1, Pin2}
import com.buransky.ostrostroj.app.device.{PinCommand, PinHigh, PinLow}
import org.slf4j.LoggerFactory

import scala.concurrent.Future
import scala.util.{Failure, Success}

object Keyboard {
  private val logger = LoggerFactory.getLogger(Keyboard.getClass)

  final case class ListenForKey()

  def apply(driver: ActorRef[PinCommand]): Behavior[ListenForKey] = Behaviors.setup { ctx =>
    ctx.self ! ListenForKey()
    Behaviors.receiveMessage {
      case ListenForKey() =>
        val futureKey = Future(Console.in.read())(ctx.executionContext)
        ctx.pipeToSelf(futureKey) {
          case Success(key) =>
            key match {
              case 'R' => driver ! PinCommand(Pin0, PinHigh)
              case 'G' => driver ! PinCommand(Pin1, PinHigh)
              case 'B' => driver ! PinCommand(Pin2, PinHigh)

              case 'r' => driver ! PinCommand(Pin0, PinLow)
              case 'g' => driver ! PinCommand(Pin1, PinLow)
              case 'b' => driver ! PinCommand(Pin2, PinLow)
              case _ =>
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
