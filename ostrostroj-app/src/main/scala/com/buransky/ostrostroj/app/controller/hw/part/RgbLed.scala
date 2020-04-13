package com.buransky.ostrostroj.app.controller.hw.part

import akka.actor.typed.Behavior
import akka.actor.typed.scaladsl.Behaviors
import com.buransky.ostrostroj.app.controller.hw.OdroidC2Driver.Gpio.{GpioPin, PinState}

object RgbLed {
  final case class Config(rPin: GpioPin, gPin: GpioPin, bPin: GpioPin)
  final case class Color(r: PinState, g: PinState, b: PinState)

  def apply(config: Config): Behavior[Color] = Behaviors.ignore
}
