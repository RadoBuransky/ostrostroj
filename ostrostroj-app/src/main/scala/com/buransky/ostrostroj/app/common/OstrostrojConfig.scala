package com.buransky.ostrostroj.app.common

import java.nio.file.{Path, Paths}

import com.google.common.base.Preconditions
import com.typesafe.config.{Config, ConfigFactory, ConfigValue, ConfigValueFactory}
import org.slf4j.LoggerFactory

import scala.collection.StringOps
import scala.jdk.CollectionConverters._

object OstrostrojConfig {
  private val logger = LoggerFactory.getLogger(OstrostrojConfig.getClass)

  val ACTOR_SYSTEM_NAME = "ostrostroj"

  // Akka cluster roles:
  val DEV_DESKTOP = "dev-desktop"
  val DEV_DEVICE = "dev-device"

  // Path to playlist
  val OSTROSTROJ_PLAYLIST = "OSTROSTROJ_PLAYLIST"

  lazy val config: Config = {
    val configMap = Map("akka.cluster.roles" -> clusterRoles().asJava)
    val dynamicConfig = ConfigFactory.parseMap(configMap.asJava)
    val defaultConfig = ConfigFactory.load()
    val devConfig = defaultConfig.getConfig("dev")

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
  val isDevDevice: Boolean = System.getProperty(DEV_DEVICE) != null
  val playlistPath: Path = {
    val playlistPathEnv = Preconditions.checkNotNull(System.getenv(OSTROSTROJ_PLAYLIST),
      OSTROSTROJ_PLAYLIST.asInstanceOf[AnyRef])
    Paths.get(playlistPathEnv)
  }
  val develeoperMode: Boolean = isDevDesktop || isDevDevice

  val ostrostroj: Config = config.getConfig("ostrostroj")
  val audio: Config = ostrostroj.getConfig("audio")

  if (logger.isDebugEnabled) {
    logger.debug(s"isDevDesktop = $isDevDesktop")
    logger.debug(s"isDevDevice = $isDevDevice")
    logger.debug(s"playlistPath = $playlistPath")
    logger.debug(s"develeoperMode = $develeoperMode")
  }

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
