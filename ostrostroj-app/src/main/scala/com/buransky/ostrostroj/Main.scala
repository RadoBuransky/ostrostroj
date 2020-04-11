package com.buransky.ostrostroj

import com.pi4j.io.gpio.{GpioFactory, OdroidC1Pin, PinState}
import com.pi4j.platform.{Platform, PlatformManager}
import org.slf4j.LoggerFactory

/**
 * Ostrostroj App entry point.
 */
object Main {
  private val logger = LoggerFactory.getLogger(this.getClass)

  def main(args: Array[String]): Unit = {
    try {
      PlatformManager.setPlatform(Platform.ODROID)
      val gpio = GpioFactory.getInstance()
      val pin0 = gpio.provisionDigitalOutputPin(OdroidC1Pin.GPIO_00, "B", PinState.LOW)
      val pin1 = gpio.provisionDigitalOutputPin(OdroidC1Pin.GPIO_01, "R", PinState.LOW)
      val pin2 = gpio.provisionDigitalOutputPin(OdroidC1Pin.GPIO_02, "G", PinState.LOW)
      pin0.setShutdownOptions(false, PinState.LOW)
      pin1.setShutdownOptions(false, PinState.LOW)
      pin2.setShutdownOptions(false, PinState.LOW)
      pin0.high()
      Thread.sleep(1000)
      pin0.low()
      pin1.high()
      Thread.sleep(1000)
      pin1.low()
      pin2.high()
      Thread.sleep(1000)
    }
    catch {
      case ex: Throwable =>
        logger.error("Ostrostroj crashed!", ex)
        throw ex
    }
  }
}
