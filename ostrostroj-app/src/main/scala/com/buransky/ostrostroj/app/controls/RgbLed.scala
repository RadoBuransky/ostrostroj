package com.buransky.ostrostroj.app.controls

import akka.actor.typed.scaladsl.Behaviors
import akka.actor.typed.{ActorRef, Behavior}
import com.buransky.ostrostroj.app.device
import com.buransky.ostrostroj.app.device.{GpioPin, Pin0, Pin1, Pin2, PinCommand}

object RgbLed {
  sealed trait Config {
    def redGpioPin: GpioPin
    def greenGpioPin: GpioPin
    def blueGpioPin: GpioPin
  }
  object Led1 extends Config {
    override val redGpioPin: GpioPin = Pin0
    override val greenGpioPin: GpioPin = Pin1
    override val blueGpioPin: GpioPin = Pin2
  }
  object Led2 extends Config {
    override val redGpioPin: GpioPin = ???
    override val greenGpioPin: GpioPin = ???
    override val blueGpioPin: GpioPin = ???
  }
  object Led3 extends Config {
    override val redGpioPin: GpioPin = ???
    override val greenGpioPin: GpioPin = ???
    override val blueGpioPin: GpioPin = ???
  }
  object Led4 extends Config {
    override val redGpioPin: GpioPin = ???
    override val greenGpioPin: GpioPin = ???
    override val blueGpioPin: GpioPin = ???
  }

  final case class Color(red: Boolean, green: Boolean, blue: Boolean)

  def apply(pinDriver: ActorRef[PinCommand], config: Config): Behavior[Color] = Behaviors.receive{ (ctx, msg) =>
    pinDriver ! device.PinCommand(config.redGpioPin, msg.red)
    pinDriver ! device.PinCommand(config.greenGpioPin, msg.green)
    pinDriver ! device.PinCommand(config.blueGpioPin, msg.blue)
    Behaviors.same
  }
}
