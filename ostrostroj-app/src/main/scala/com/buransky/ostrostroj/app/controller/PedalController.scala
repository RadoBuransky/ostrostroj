package com.buransky.ostrostroj.app.controller

import java.util

import akka.actor.typed.scaladsl.{AbstractBehavior, ActorContext, Behaviors}
import akka.actor.typed.{ActorRef, Behavior}
import com.buransky.max7219
import com.buransky.max7219.Max7219.BitChange
import com.buransky.max7219.register._
import com.buransky.max7219.{LedMatrix, Max7219}
import com.buransky.ostrostroj.app.common.{OstrostrojConfig, OstrostrojException}
import com.buransky.ostrostroj.app.device._

import scala.jdk.CollectionConverters._

/**
 * Logical API for physical floor pedal controller (buttons, LEDs, ...).
 */
object PedalController {
  private val LOADCS_PIN = Pin4;
  private val CLK_PIN = Pin3;
  private val DIN_PIN = Pin5;

  sealed trait ControllerCommand
  final case class LedControllerCommand(led: Model.Led, ledCommand: Model.LedColor) extends ControllerCommand
  final case class LedMatrixRegister(register: max7219.Register) extends ControllerCommand
  final case class LedMatrixDrawPoint(row: Int, column: Int, ledOn: Boolean)
  final case class LedMatrixDraw(leds: Iterable[LedMatrixDrawPoint]) extends ControllerCommand

  sealed trait ControllerEvent

  def apply(driver: ActorRef[DriverCommand]): Behavior[ControllerCommand] = Behaviors.setup { ctx =>
    new PedalControllerBehavior(driver, ctx)
  }

  class PedalControllerBehavior(driver: ActorRef[DriverCommand],
                                ctx: ActorContext[ControllerCommand]) extends AbstractBehavior[ControllerCommand](ctx) {
    private val led1 = ctx.spawn(RgbLed(driver, RgbLed.Config(Pin0, Pin1, Pin2)), "led1")
    private val ledMatrix = createLedMatrix()

    if (OstrostrojConfig.develeoperMode) {
      ctx.spawn(Keyboard(driver, ctx.self), "keyboard")
    }

    override def onMessage(message: ControllerCommand): Behavior[ControllerCommand] = {
      message match {
        case LedControllerCommand(led, ledColor) =>
          // Translate logical command to physical
          hwLedActorRef(led) ! hwLedCommand(ledColor)
          Behaviors.same
        case LedMatrixRegister(register) =>
          driver ! enqueueLedMatrixResult(ledMatrix.executeAll(register))
          Behaviors.same
        case LedMatrixDraw(leds) =>
          leds.foreach(l => ledMatrix.setLedStatus(l.row, l.column, l.ledOn))
          driver ! enqueueLedMatrixResult(ledMatrix.draw())
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
    private def hwLedCommand(ledColor: Model.LedColor): RgbLed.Color = {
      RgbLed.Color(ledColor.r, ledColor.g, ledColor.b)
    }

    private def createLedMatrix(): LedMatrix = {
      val result = Max7219.initLedMatrix(8, 8, 1, 4)
      driver ! StartSpi(0, 1)
      driver ! enqueueLedMatrixResult(result.executeAll(ShutdownRegister.NormalOperation))
      driver ! enqueueLedMatrixResult(result.executeAll(ScanLimitRegister.Digits0to7))
      driver ! enqueueLedMatrixResult(result.executeAll(DecodeModeRegister.NoDecode))
      driver ! enqueueLedMatrixResult(result.executeAll(DisplayTestRegister.NormalOperation))
      driver ! enqueueLedMatrixResult(result.executeAll(new IntensityRegister(0)))
      result
    }

    private def enqueueLedMatrixResult(result: java.lang.Iterable[BitChange]): EnqueueToSpi = {
      val pinCommands = result.asScala.map {
        case BitChange.LOADCS_HIGH => PinCommand(LOADCS_PIN, state = true);
        case BitChange.LOADCS_LOW => PinCommand(LOADCS_PIN, state = false);
        case BitChange.CLK_HIGH => PinCommand(CLK_PIN, state = true);
        case BitChange.CLK_LOW => PinCommand(CLK_PIN, state = false);
        case BitChange.DIN_HIGH => PinCommand(DIN_PIN, state = true);
        case BitChange.DIN_LOW => PinCommand(DIN_PIN, state = false);
        case other => throw new OstrostrojException(s"Unknown bit change! [$other]")
      }
      EnqueueToSpi(0, pinCommands)
    }
  }
}