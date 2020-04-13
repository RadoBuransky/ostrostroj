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

  def apply(): Behavior[PinCommand] = Behaviors.setup(createBehavior)

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

  private def createBehavior(context: ActorContext[PinCommand]): Behavior[PinCommand] = {
    try {
      new GpioBehavior(context)
    }
    catch {
      case ex: UnsatisfiedLinkError if ex.getMessage == "com.pi4j.wiringpi.Gpio.wiringPiSetup()I" =>
        logger.warn(s"WiringPI failed to start. Continuing with emulator.")
        logger.debug("WiringPI failure details", ex)
        new EmulatorBehavior(context)
      case t: Throwable => throw t
    }
  }

  class EmulatorBehavior(context: ActorContext[PinCommand]) extends AbstractBehavior[PinCommand](context) {
    override def onMessage(msg: PinCommand): Behavior[PinCommand] = Behaviors.ignore
  }
}
