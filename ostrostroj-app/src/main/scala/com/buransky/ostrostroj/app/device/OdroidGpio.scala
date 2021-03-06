package com.buransky.ostrostroj.app.device

import akka.actor.typed.receptionist.{Receptionist, ServiceKey}
import akka.actor.typed.scaladsl.{AbstractBehavior, ActorContext, Behaviors}
import akka.actor.typed.{Behavior, PostStop, Signal}
import com.buransky.ostrostroj.app.common.{OstrostrojException, OstrostrojMessage}
import com.pi4j.io.gpio._
import com.pi4j.platform.{Platform, PlatformManager}
import org.slf4j.LoggerFactory

import scala.collection.mutable

// Low-level API for physical device
sealed trait DriverCommand extends OstrostrojMessage

// Direct GPIO pin commands
sealed class GpioPin(val pi4jPinAddress: Int) extends OstrostrojMessage
case object Pin0 extends GpioPin(OdroidC1Pin.GPIO_00.getAddress)
case object Pin1 extends GpioPin(OdroidC1Pin.GPIO_01.getAddress)
case object Pin2 extends GpioPin(OdroidC1Pin.GPIO_02.getAddress)
case object Pin3 extends GpioPin(OdroidC1Pin.GPIO_03.getAddress)
case object Pin4 extends GpioPin(OdroidC1Pin.GPIO_04.getAddress)
case object Pin5 extends GpioPin(OdroidC1Pin.GPIO_05.getAddress)
final case class PinCommand(pin: GpioPin, state: Boolean) extends DriverCommand

// SPI commands
final case class StartSpi(spiId: Int, periodNs: Int) extends DriverCommand
final case class EnqueueToSpi(spiId: Int, spiData: Iterable[PinCommand]) extends DriverCommand

/**
 * Low-level driver for Odroid C2.
 */
object OdroidGpio {
  // Static initialization of Pi4j
  PlatformManager.setPlatform(Platform.ODROID)

  val odroidGpioKey: ServiceKey[DriverCommand] = ServiceKey[DriverCommand]("OdroidGpio")
  private val logger = LoggerFactory.getLogger(OdroidGpio.getClass)
  private val spis = mutable.Map[Int, SpiQueue]()

  def apply(): Behavior[DriverCommand] = Behaviors.setup { ctx =>
    ctx.system.receptionist ! Receptionist.Register(odroidGpioKey, ctx.self)
    logger.debug("OdroidGpio registered to receptionist.")
    new GpioBehavior(ctx)
  }

  class GpioBehavior(context: ActorContext[DriverCommand]) extends AbstractBehavior[DriverCommand](context) {
    private val gpio: GpioController = {
      logger.debug("Initializing PI4J GPIO...")
      val result = try {
        GpioFactory.getInstance()
      }
      catch {
        case ex: UnsatisfiedLinkError =>
          throw new OstrostrojException("Are you sure this is running on Odroid C2 using JDK built for ARM-HF?", ex)
      }
      logger.info("PI4J GPIO initialized.")
      result
    }

    private val digitalOutputPins: Map[Int, GpioPinDigitalOutput] = {
      List(
        OdroidC1Pin.GPIO_00,
        OdroidC1Pin.GPIO_01,
        OdroidC1Pin.GPIO_02,
        OdroidC1Pin.GPIO_03,
        OdroidC1Pin.GPIO_04,
        OdroidC1Pin.GPIO_05
      ).map { pin =>
        logger.debug(s"Provisioning digital output pin ${pin.getAddress}:${pin.getName}...")
        val result = gpio.provisionDigitalOutputPin(pin, PinState.LOW)
        result.setShutdownOptions(true, PinState.LOW)
        logger.debug(s"Pin ${pin.getAddress}:${pin.getName} provisioned.")
        pin.getAddress -> result
      }.toMap
    }

    override def onMessage(msg: DriverCommand): Behavior[DriverCommand] = msg match {
      case pinCommand: PinCommand =>
        commandExecutor(pinCommand)
        Behaviors.same
      case StartSpi(spiId, periodNs) =>
        val spiQueue = new SpiQueue(commandExecutor, periodNs)
        spis.addOne(spiId -> spiQueue)
        Behaviors.same
      case EnqueueToSpi(spiId, spiData) =>
        spis(spiId).enqueue(spiData)
        Behaviors.same
    }

    override def onSignal: PartialFunction[Signal, Behavior[DriverCommand]] = {
      case PostStop =>
        logger.debug("Shutting down PI4J GPIO...")
        gpio.shutdown()
        logger.info("PI4J GPIO shut down.")
        Behaviors.same
    }

    private def commandExecutor(pinCommand: PinCommand): Unit = {
      stateExecutor(digitalOutputPins(pinCommand.pin.pi4jPinAddress))(pinCommand.state)
    }

    private def stateExecutor(pin: GpioPinDigitalOutput)(state: Boolean): Unit = {
      if (logger.isTraceEnabled) {
        logger.trace(s"${pin.getPin.getAddress} - ${state}")
      }
      if (state) {
        pin.high()
      } else {
        pin.low()
      }
    }
  }
}
