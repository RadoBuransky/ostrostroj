package com.buransky.ostrostroj.app.controller.hw

import akka.NotUsed
import akka.actor.typed.Behavior
import akka.actor.typed.scaladsl.Behaviors

/**
 * Low-level driver for Odroid C2.
 */
object OdroidC2Driver {
  /**
   * General-purpose input/output.
   */
  object Gpio {
    sealed abstract class GpioPin(wPi: Int)
    final case object Pin1 extends GpioPin(1)
    final case object Pin2 extends GpioPin(2)
    final case object Pin3 extends GpioPin(3)

    sealed trait PinState
    final case object PinHigh extends PinState
    final case object PinLow extends PinState

    object PinState {
      def apply(b: Boolean): PinState = if (b) PinHigh else PinLow
    }
  }

  def apply(): Behavior[NotUsed] = Behaviors.ignore
}
