package com.buransky.ostrostroj.app.common

import com.typesafe.config.{Config, ConfigFactory}

import scala.jdk.CollectionConverters._

object OstrostrojConfig {
  val ACTOR_SYSTEM_NAME = "ostrostroj"

  // Akka cluster roles:
  val DEV_DESKTOP = "dev-desktop"
  val DEV_DEVICE = "dev-device"

  lazy val config: Config = {
    val configMap = Map("akka.cluster.roles" -> clusterRoles().asJava)
    val dynamicConfig = ConfigFactory.parseMap(configMap.asJava)
    val defaultConfig = ConfigFactory.load()

    val devConfig = if (develeoperMode) {
      defaultConfig.getConfig("ostrostroj.dev").withFallback(dynamicConfig)
    }
    else {
      dynamicConfig
    }

    devConfig.withFallback(defaultConfig)
  }
  val develeoperMode: Boolean = (System.getProperty(DEV_DESKTOP) != null) || (System.getProperty(DEV_DEVICE) != null)

  private def clusterRoles(): List[String] = {
    if (System.getProperty(DEV_DESKTOP) != null) {
      List(DEV_DESKTOP)
    }
    else {
      if (System.getProperty(DEV_DEVICE) != null) {
        List(DEV_DEVICE)
      }
      else {
        Nil
      }
    }
  }
}
