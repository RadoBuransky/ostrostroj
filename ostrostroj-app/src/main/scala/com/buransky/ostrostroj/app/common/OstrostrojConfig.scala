package com.buransky.ostrostroj.app.common

import java.nio.file.{Path, Paths}

import com.typesafe.config.{Config, ConfigFactory, ConfigValueFactory}
import org.slf4j.LoggerFactory

import scala.jdk.CollectionConverters._

object OstrostrojConfig {
  private val logger = LoggerFactory.getLogger(OstrostrojConfig.getClass)

  val ACTOR_SYSTEM_NAME = "ostrostroj"

  // Akka cluster roles:
  val DEV_DESKTOP = "DEV_DESKTOP"
  val DEV_DEVICE = "DEV_DEVICE"

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

  val isDevDesktop: Boolean = System.getenv(DEV_DESKTOP) != null
  val isDevDevice: Boolean = System.getenv(DEV_DEVICE) != null
  val playlistPath: Path = {
    val ostrostrojPlaylist = System.getenv(OSTROSTROJ_PLAYLIST)
    if (ostrostrojPlaylist == null) {
      logger.error(System.getenv().asScala.map(e => s"${e._1}-${e._2}").mkString("[", ";", "]"))
      throw new IllegalArgumentException("OSTROSTROJ_PLAYLIST == null")
    } else {
      Paths.get(ostrostrojPlaylist)
    }
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
