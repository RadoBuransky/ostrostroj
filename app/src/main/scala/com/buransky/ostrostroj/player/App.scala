package com.buransky.ostrostroj.player

import com.buransky.ostrostroj.player.App.log
import org.apache.logging.log4j.LogManager
import org.slf4j.LoggerFactory

import java.util.HexFormat
import javax.sound.midi.{MidiDevice, MidiMessage, MidiSystem, Receiver, Sequencer}

class App(device: MidiDevice) extends Receiver with AutoCloseable {
  device.open()
  device.getTransmitter.setReceiver(this)
  override def close(): Unit = {
    device.close()
  }

  override def send(message: MidiMessage, timeStamp: Long): Unit = {
    log.info(s"${message.getStatus} ${HexFormat.of().formatHex(message.getMessage)}")
  }
}

object App {
  private val log = LoggerFactory.getLogger(classOf[App.type])
  private val midiDeviceName = "IMPACT LX25"

  def main(args: Array[String]): Unit = {
    log.info("Ostrostroj player started.")
    val result = try {
      Runtime.getRuntime.addShutdownHook(new Thread() {
        override def run(): Unit = {
          log.info("Shutting down...")
          synchronized {
            App.notifyAll()
          }
        }
      })
      printAllDeviceNames()
      val app = new App(findDevice())
      try {
        synchronized {
          log.info("Running...")
          wait()
        }
      } finally {
        app.close()
      }
      log.info("Ostrostroj player finished.")
      0
    } catch {
      case t: Throwable =>
        log.error("Ostrostroj failed!", t)
        Console.err.println(t.getMessage)
        t.printStackTrace(Console.err)
        1
    } finally {
      LogManager.shutdown()
    }
    System.exit(result)
  }

  private def findDevice(): MidiDevice = {
    MidiSystem.getMidiDeviceInfo.map(MidiSystem.getMidiDevice).find { device =>
      device.getDeviceInfo.getName == midiDeviceName && device.getMaxReceivers == 0 && device.getMaxTransmitters != 0
    }.get
  }

  private def printAllDeviceNames(): Unit = {
    MidiSystem.getMidiDeviceInfo.foreach { deviceInfo =>
      val device = MidiSystem.getMidiDevice(deviceInfo)
      log.info(s"${deviceInfo.getName} ${device.getMaxTransmitters} ${device.getMaxReceivers}")
    }
  }
}
