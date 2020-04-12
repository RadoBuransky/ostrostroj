package com.buransky.ostrostroj.app

import akka.NotUsed
import akka.actor.typed.scaladsl.Behaviors
import akka.actor.typed.{ActorSystem, Behavior, Terminated}
import com.buransky.ostrostroj.app.controller.PedalController
import com.typesafe.config.ConfigFactory
import org.slf4j.LoggerFactory

/**
 * Ostrostroj App entry point.
 */
object Main {
  private val config = ConfigFactory.load()
  private val logger = LoggerFactory.getLogger(this.getClass)

  def apply(): Behavior[NotUsed] = Behaviors.setup { context =>
    val controller = context.spawn(PedalController(), "controller")

    Behaviors.receiveSignal {
      case (_, Terminated(_)) =>
        Behaviors.stopped
    }
  }

  def main(args: Array[String]): Unit = {
    ActorSystem(Main(), "ostrostroj", config)
  }
}
