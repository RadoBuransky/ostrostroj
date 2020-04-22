package com.buransky.ostrostroj.app.controls

import akka.actor.typed.scaladsl.Behaviors
import akka.actor.typed.{ActorRef, Behavior}
import com.buransky.ostrostroj.app.device.{DriverCommand, GpioPin}

object Button {
  sealed trait Config {
    def gpioPin: GpioPin
  }
  object Button1 extends Config {
    override val gpioPin: GpioPin = ???
  }
  object Button2 extends Config {
    override val gpioPin: GpioPin = ???
  }
  object Button3 extends Config {
    override val gpioPin: GpioPin = ???
  }
  object Button4 extends Config {
    override val gpioPin: GpioPin = ???
  }

  sealed trait ButtonEvent
  final case class ButtonDown(button: Config) extends ButtonEvent
  final case class ButtonUp(button: Config) extends ButtonEvent
  final case class ButtonLongPress(button: Config) extends ButtonEvent

  final case class Subscribe(listener: ActorRef[ButtonEvent], button: Config)

  def apply(driver: ActorRef[DriverCommand]): Behavior[Subscribe] = Behaviors.ignore
}
