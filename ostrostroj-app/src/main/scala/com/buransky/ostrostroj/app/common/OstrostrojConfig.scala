package com.buransky.ostrostroj.app.common

import com.typesafe.config.{Config, ConfigFactory, ConfigValue, ConfigValueFactory}

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
    val devConfig = defaultConfig.getConfig("ostrostroj.dev")

    val mergedConfig = if (develeoperMode) {
      val hostname = if (isDevDesktop) {
        devConfig.getString("desktopHostname")
      } else {
        devConfig.getString("deviceHostname")
      }

      devConfig
        .withValue("akka.remote.artery.canonical.hostname", ConfigValueFactory.fromAnyRef(hostname))
        .withFallback(dynamicConfig)
    }
    else {
      dynamicConfig
    }

    mergedConfig.withFallback(defaultConfig)
  }

  val isDevDesktop: Boolean = System.getProperty(DEV_DESKTOP) != null
  val isDevDevice: Boolean = System.getProperty(DEV_DESKTOP) != null
  val develeoperMode: Boolean = isDevDesktop || isDevDevice

  private def clusterRoles(): List[String] = {
    if (isDevDesktop) {
      List(DEV_DESKTOP)
    }
    else {
      if (isDevDevice) {
        List(DEV_DEVICE)
      }
      else {
        Nil
      }
    }
  }
}
