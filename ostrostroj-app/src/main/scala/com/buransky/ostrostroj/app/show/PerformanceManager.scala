package com.buransky.ostrostroj.app.show

import akka.NotUsed
import akka.actor.typed.Behavior
import akka.actor.typed.scaladsl.Behaviors

/**
 * Stateful orchestrator of a single performance. The main guy during the show.
 */
object PerformanceManager {
  def apply(): Behavior[NotUsed] = Behaviors.ignore
}
