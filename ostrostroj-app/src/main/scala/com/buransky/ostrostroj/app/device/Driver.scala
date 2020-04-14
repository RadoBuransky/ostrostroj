package com.buransky.ostrostroj.app.device

import com.buransky.ostrostroj.app.device.Gpio.GpioPin
import com.pi4j.io.gpio.{OdroidC1Pin, Pin}

/**
 * Abstract API for physical hardware.
 */
object Gpio {
  sealed abstract class GpioPin(val pi4jPin: Pin)
  final case object Pin0 extends GpioPin(OdroidC1Pin.GPIO_00)
  final case object Pin1 extends GpioPin(OdroidC1Pin.GPIO_01)
  final case object Pin2 extends GpioPin(OdroidC1Pin.GPIO_02)
}

sealed trait DigitalPinState
case object PinHigh extends DigitalPinState
case object PinLow extends DigitalPinState

object DigitalPinState {
  def apply(b: Boolean): DigitalPinState = if (b) PinHigh else PinLow
}

final case class PinCommand(pin: GpioPin, state: DigitalPinState)