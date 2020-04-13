package com.buransky.ostrostroj.app.controller.hw.part

import akka.actor.typed.scaladsl.Behaviors
import akka.actor.typed.{ActorRef, Behavior}
import com.buransky.ostrostroj.app.controller.hw.Gpio.GpioPin
import com.buransky.ostrostroj.app.controller.hw.{DigitalPinState, PinCommand}

object RgbLed {
  final case class Config(rPin: GpioPin, gPin: GpioPin, bPin: GpioPin)
  final case class Color(r: DigitalPinState, g: DigitalPinState, b: DigitalPinState)

  def apply(pinDriver: ActorRef[PinCommand], config: Config): Behavior[Color] = Behaviors.ignore
}
