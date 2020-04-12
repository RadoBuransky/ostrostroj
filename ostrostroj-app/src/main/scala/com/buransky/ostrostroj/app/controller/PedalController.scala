package com.buransky.ostrostroj.app.controller

import akka.NotUsed
import akka.actor.typed.Behavior
import akka.actor.typed.scaladsl.Behaviors

/**
 * Physical floor pedal controller (buttons, LEDs, ...).
 */
object PedalController {
  def apply(): Behavior[NotUsed] = Behaviors.ignore

}
