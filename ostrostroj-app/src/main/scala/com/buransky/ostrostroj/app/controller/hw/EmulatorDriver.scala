package com.buransky.ostrostroj.app.controller.hw

import akka.actor.typed.Behavior
import akka.actor.typed.scaladsl.Behaviors

/**
 * Emulator driver.
 */
object EmulatorDriver {
  def apply(): Behavior[PinCommand] = Behaviors.ignore
}
