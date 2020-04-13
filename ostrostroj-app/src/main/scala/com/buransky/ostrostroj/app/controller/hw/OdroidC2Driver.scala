package com.buransky.ostrostroj.app.controller.hw

import akka.actor.typed.scaladsl.{AbstractBehavior, ActorContext, Behaviors}
import akka.actor.typed.{Behavior, PostStop, Signal}
import com.pi4j.io.gpio._
import com.pi4j.platform.{Platform, PlatformManager}
import org.slf4j.LoggerFactory

/**
 * Low-level driver for Odroid C2.
 */
object OdroidC2Driver {
  private val logger = LoggerFactory.getLogger(OdroidC2Driver.getClass)

  def apply(): Behavior[PinCommand] = Behaviors.setup(ctx => new GpioBehavior(ctx))

  class GpioBehavior(context: ActorContext[PinCommand]) extends AbstractBehavior[PinCommand](context) {
    private val gpio: GpioController = {
      logger.debug("Initializing PI4J GPIO...")
      PlatformManager.setPlatform(Platform.ODROID)
      val result = GpioFactory.getInstance()
      logger.info("PI4J GPIO initialized.")
      result
    }

    private val digitalOutputPins: Map[Pin, GpioPinDigitalOutput] = {
      OdroidC1Pin.allPins(PinMode.DIGITAL_OUTPUT).map { pin =>
        val result = gpio.provisionDigitalOutputPin(pin, PinState.LOW)
        result.setShutdownOptions(true, PinState.LOW)
        pin -> result
      }.toMap
    }

    override def onMessage(message: PinCommand): Behavior[PinCommand] = {
      message.state match {
        case PinHigh => digitalOutputPins(message.pin.pi4jPin).high()
        case PinLow => digitalOutputPins(message.pin.pi4jPin).low()
      }
      Behaviors.same
    }

    override def onSignal: PartialFunction[Signal, Behavior[PinCommand]] = {
      case PostStop =>
        logger.debug("Shutting down PI4J GPIO...")
        gpio.shutdown()
        logger.info("PI4J GPIO shut down.")
        Behaviors.stopped
    }
  }
}
