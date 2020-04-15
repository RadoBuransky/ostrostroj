package com.buransky.ostrostroj.app.controller

import akka.actor.typed.scaladsl.Behaviors
import akka.actor.typed.{ActorRef, Behavior}
import com.buransky.ostrostroj.app.device.Gpio.GpioPin
import com.buransky.ostrostroj.app.device.{DigitalPinState, PinCommand}

object Max7219 {
  final case class Config(dinPin: GpioPin, csPin: GpioPin, clkPin: GpioPin)
  case class Command(din: DigitalPinState, cs: DigitalPinState, clk: DigitalPinState)

  def apply(pinDriver: ActorRef[PinCommand], config: Config): Behavior[Max7219.Command] = Behaviors.receiveMessage {
    case Command(din, cs, clk) =>
      pinDriver ! PinCommand(config.dinPin, din)
      pinDriver ! PinCommand(config.csPin, cs)
      pinDriver ! PinCommand(config.clkPin, clk)
      Behaviors.same
  }
}
