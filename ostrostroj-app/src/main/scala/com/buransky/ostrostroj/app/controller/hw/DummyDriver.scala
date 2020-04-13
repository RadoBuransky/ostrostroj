package com.buransky.ostrostroj.app.controller.hw

import akka.actor.typed.Behavior
import akka.actor.typed.scaladsl.Behaviors

/**
 * Dummy driver.
 */
object DummyDriver {
  def apply(): Behavior[PinCommand] = Behaviors.ignore
}
