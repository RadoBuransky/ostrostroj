package com.buransky.ostrostroj.player

import com.buransky.ostrostroj.player.App.log
import org.apache.logging.log4j.LogManager
import org.slf4j.LoggerFactory

import java.util.HexFormat
import javax.sound.midi._

// https://www.lim.di.unimi.it/IEEE/MIDI/SPECI.HTM
class App(device: MidiDevice) extends MetaEventListener with ControllerEventListener with Receiver with AutoCloseable {
  device.open()
  log.info(s"MIDI device ${device.getDeviceInfo.getName} open...")
  device.getTransmitter.setReceiver(this)

  override def close(): Unit = {
    device.close()
  }

  override def send(message: MidiMessage, timeStamp: Long): Unit = {
    // https://stackoverflow.com/a/15993963
    message.command match {
      case ShortMessage.TIMING_CLOCK =>
        log.debug("tick")
      case ShortMessage.START =>
        log.info("start")
      case ShortMessage.CONTINUE =>
        log.info("continue")
      case ShortMessage.STOP =>
        log.info("stop")
      case ShortMessage.CONTROL_CHANGE =>
        log.info(s"ch${message.channel.get} CC")
      case ShortMessage.PROGRAM_CHANGE =>
        val sm = message.asInstanceOf[ShortMessage]
        log.info(s"ch${sm.channel.get} PC ${sm.getData1} ${sm.getData2}")
      case _ =>
        log.info(s"${message.getStatus} ${HexFormat.of().formatHex(message.getMessage)}")
    }
  }

  override def controlChange(event: ShortMessage): Unit = {
    log.info(s"ch${event.getChannel} ${event.getCommand} ${HexFormat.of().formatHex(event.getMessage)}")
  }

  override def meta(meta: MetaMessage): Unit = {
    log.info(s"${meta.getType} ${HexFormat.of().formatHex(meta.getMessage)}")
  }
}

object App {
  private val log = LoggerFactory.getLogger(classOf[App.type])
  private val midiDeviceName = "ESI MIDIMATE eX"

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
