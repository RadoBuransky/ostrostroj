package com.buransky.ostrostroj.app

import akka.actor.typed.ActorSystem
import com.buransky.ostrostroj.app.common.OstrostrojConfig
import com.buransky.ostrostroj.app.common.OstrostrojConfig._
import org.slf4j.LoggerFactory

/**
 * Ostrostroj App entry point.
 */
object Main {
  private val logger = LoggerFactory.getLogger(this.getClass)

  def main(args: Array[String]): Unit = {
    initLogging()
    if (OstrostrojConfig.develeoperMode) {
      logger.warn("Ostrostroj running in developer mode using Akka cluster.")
    } else {
      logger.info(s"Ostrostroj running in production mode.")
    }
    ActorSystem(OstrostrojApp(), ACTOR_SYSTEM_NAME, config)
  }

  private def initLogging(): Unit = {
    import org.slf4j.bridge.SLF4JBridgeHandler
    SLF4JBridgeHandler.removeHandlersForRootLogger()
    SLF4JBridgeHandler.install()
  }
}
