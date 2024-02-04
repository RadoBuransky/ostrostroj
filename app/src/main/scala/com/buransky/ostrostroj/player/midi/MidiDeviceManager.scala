package com.buransky.ostrostroj.player.midi

import com.buransky.ostrostroj.player.midi.MidiDeviceManager.log
import org.slf4j.LoggerFactory

import java.time.Duration
import java.util.concurrent.{Executors, ScheduledExecutorService, TimeUnit}
import javax.sound.midi.{MidiDevice, MidiSystem}
import scala.collection.mutable
import scala.util.control.NonFatal

class MidiDeviceManager(ostrostrojPlayer: MidiCommands,
                        schedulerThreadPool: ScheduledExecutorService) extends Runnable with AutoCloseable {
  private val devices = new mutable.HashMap[String, MidiReceiver]()

  override def run(): Unit = synchronized {
    try {
      val deviceInfos = MidiSystem.getMidiDeviceInfo
      addDevices(deviceInfos)
      removeDevices(deviceInfos)
    } catch {
      case NonFatal(ex) =>
        log.error("Refresh failed!", ex)
    }
  }

  override def close(): Unit = synchronized {
    schedulerThreadPool.shutdown()
    devices.foreach(_._2.close())
    devices.clear()
  }

  private def addDevices(deviceInfos: Iterable[MidiDevice.Info]): Unit = {
    deviceInfos.foreach { deviceInfo =>
      try {
        if (!devices.contains(deviceInfo.getName)) {
          val device = MidiSystem.getMidiDevice(deviceInfo)
          if (device.getMaxReceivers == 0 && device.getMaxTransmitters != 0) {
            addDevice(device)
          }
        }
      } catch {
        case NonFatal(ex) =>
          log.error(s"Add failed! [${deviceInfo.getName}]", ex)
      }
    }
  }

  private def addDevice(device: MidiDevice): Unit = {
    val receiver = new MidiReceiver(device, ostrostrojPlayer)
    device.open()
    device.getTransmitter.setReceiver(receiver)
    devices.put(device.getDeviceInfo.getName, receiver)
    log.info(s"${device.getDeviceInfo.getName} added.")
  }

  private def removeDevices(deviceInfos: Iterable[MidiDevice.Info]): Unit = {
    devices.filter(kv => !deviceInfos.exists(_.getName == kv._1)).foreach { case (name, device) =>
      try {
        devices.remove(name)
        log.info(s"$name removed.")
        device.close()
      } catch {
        case NonFatal(ex) =>
          log.error(s"Remove failed! [$name]", ex)
      }
    }
  }
}

object MidiDeviceManager {
  private val log = LoggerFactory.getLogger(classOf[MidiDeviceManager])
  private val period = Duration.ofSeconds(1)

  def apply(ostrostrojPlayer: MidiCommands): MidiDeviceManager = {
    val schedulerThreadPool = Executors.newScheduledThreadPool(1)
    val result = new MidiDeviceManager(ostrostrojPlayer, schedulerThreadPool)
    schedulerThreadPool.scheduleAtFixedRate(result, 0, period.getSeconds, TimeUnit.SECONDS)
    log.info(s"MIDI device manager started. ($period)")
    result
  }
}
