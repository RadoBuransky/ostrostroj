package com.buransky.ostrostroj.app.controller

import akka.actor.typed.scaladsl.Behaviors
import akka.actor.typed.{ActorRef, Behavior}
import com.buransky.max7219.Register
import com.buransky.max7219.register.{DisplayTestRegister, IntensityRegister}
import com.buransky.ostrostroj.app.controller.PedalController.{ControllerCommand, LedMatrixDraw, LedMatrixDrawPoint, LedMatrixRegister}
import com.buransky.ostrostroj.app.device._
import org.slf4j.LoggerFactory

import scala.concurrent.Future
import scala.util.{Failure, Success}

object Keyboard {
  private val logger = LoggerFactory.getLogger(Keyboard.getClass)

  final case class ListenForKey()

  def apply(driver: ActorRef[DriverCommand], pedalController: ActorRef[ControllerCommand]): Behavior[ListenForKey] =
    Behaviors.setup { ctx =>
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
                case l if l.startsWith("i") => pedalController ! LedMatrixRegister(new IntensityRegister(l.tail.toByte))
                case "T" => pedalController ! LedMatrixRegister(DisplayTestRegister.DisplayTestMode)
                case "t" => pedalController ! LedMatrixRegister(DisplayTestRegister.NormalOperation)
                case "d" =>
                  for (i <- 0 to 7) {
                    pedalController ! LedMatrixDraw(List(LedMatrixDrawPoint(i, i, true)))
                  }
                case _ =>
                  val parts = line.split('.')
                  if (parts.length == 3) {
                    val row = parts(0).toInt
                    val column = parts(1).toInt
                    val ledOn: Boolean = parts(2).toInt > 0
                    pedalController ! LedMatrixDraw(List(LedMatrixDrawPoint(row, column, ledOn)))
                  } else {
                    if (parts.length == 2) {
                      val address = parts(0).toByte
                      val data = parts(1).toByte
                      pedalController ! LedMatrixRegister(InternalRegister(address, data))
                    }
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

private case class InternalRegister(address: Byte, data: Byte) extends Register {
  override def getAddress: Byte = address
  override def getData: Byte = data
}
