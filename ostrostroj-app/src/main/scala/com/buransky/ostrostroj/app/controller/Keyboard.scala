package com.buransky.ostrostroj.app.controller

import akka.actor.typed.scaladsl.Behaviors
import akka.actor.typed.{ActorRef, Behavior}
import com.buransky.ostrostroj.app.device._
import org.slf4j.LoggerFactory

import scala.concurrent.Future
import scala.util.{Failure, Success}

object Keyboard {
  private val logger = LoggerFactory.getLogger(Keyboard.getClass)

  final case class ListenForKey()

  def apply(driver: ActorRef[DriverCommand],
            ledMatrix: ActorRef[LedMatrix.LedMatrixCommand]): Behavior[ListenForKey] = Behaviors.setup { ctx =>
    ctx.self ! ListenForKey()
    Behaviors.receiveMessage {
      case ListenForKey() =>
        val futureLine = Future(Console.in.readLine())(ctx.executionContext)
        ctx.pipeToSelf(futureLine) {
          case Success(line) =>
            line match {
              case "R" => driver ! PinCommand(Pin2, true)
              case "G" => driver ! PinCommand(Pin1, true)
              case "B" => driver ! PinCommand(Pin0, true)

              case "r" => driver ! PinCommand(Pin2, false)
              case "g" => driver ! PinCommand(Pin1, false)
              case "b" => driver ! PinCommand(Pin0, false)
              case _ =>
                try {
                  val values = line.split('-').map(s => Integer.parseInt(s).toByte)
                  if (values.length == 3) {
                    driver ! Word(values(0), values(1), values(2))
                  }
                }
                catch {
                  case ex: Exception => logger.warn("", ex)
                }
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
