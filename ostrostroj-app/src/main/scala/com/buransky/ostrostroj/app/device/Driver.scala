package com.buransky.ostrostroj.app.device

import com.buransky.ostrostroj.app.common.OstrostrojMessage
import com.buransky.ostrostroj.app.device.Gpio.GpioPin
import com.pi4j.io.gpio.{OdroidC1Pin, Pin}

/**
 * Abstract API for physical hardware.
 */
object Gpio {
  sealed class GpioPin(val pi4jPinAddress: Int) extends OstrostrojMessage
  final case object Pin0 extends GpioPin(OdroidC1Pin.GPIO_00.getAddress)
  final case object Pin1 extends GpioPin(OdroidC1Pin.GPIO_01.getAddress)
  final case object Pin2 extends GpioPin(OdroidC1Pin.GPIO_02.getAddress)
  final case object Pin3 extends GpioPin(OdroidC1Pin.GPIO_03.getAddress)
  final case object Pin4 extends GpioPin(OdroidC1Pin.GPIO_04.getAddress)
  final case object Pin5 extends GpioPin(OdroidC1Pin.GPIO_05.getAddress)
}
final case class PinCommand(pin: GpioPin, state: Boolean) extends OstrostrojMessage