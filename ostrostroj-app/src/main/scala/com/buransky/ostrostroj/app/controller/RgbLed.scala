package com.buransky.ostrostroj.app.controller

import akka.actor.typed.scaladsl.Behaviors
import akka.actor.typed.{ActorRef, Behavior}
import com.buransky.ostrostroj.app.device
import com.buransky.ostrostroj.app.device.{GpioPin, PinCommand}

object RgbLed {
  final case class Config(rPin: GpioPin, gPin: GpioPin, bPin: GpioPin)
  final case class Color(r: Boolean, g: Boolean, b: Boolean)

  def apply(pinDriver: ActorRef[PinCommand], config: Config): Behavior[Color] = Behaviors.receive{ (ctx, msg) =>
    pinDriver ! device.PinCommand(config.rPin, msg.r)
    pinDriver ! device.PinCommand(config.gPin, msg.g)
    pinDriver ! device.PinCommand(config.bPin, msg.b)
    Behaviors.same
  }
}
