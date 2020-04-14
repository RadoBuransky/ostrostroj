package com.buransky.ostrostroj.app.device

import com.buransky.ostrostroj.app.common.OstrostrojMessage
import com.buransky.ostrostroj.app.device.Gpio.GpioPin
import com.pi4j.io.gpio.{OdroidC1Pin, Pin}

/**
 * Abstract API for physical hardware.
 */
object Gpio {
  sealed class GpioPin(val pi4jPin: Pin) extends OstrostrojMessage
  final case object Pin0 extends GpioPin(OdroidC1Pin.GPIO_00)
  final case object Pin1 extends GpioPin(OdroidC1Pin.GPIO_01)
  final case object Pin2 extends GpioPin(OdroidC1Pin.GPIO_02)
}

final case class DigitalPinState(high: Boolean) extends OstrostrojMessage

final case class PinCommand(pin: GpioPin, state: DigitalPinState) extends OstrostrojMessage