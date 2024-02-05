package com.buransky.ostrostroj.player

import com.buransky.ostrostroj.player.midi.MidiDeviceManager
import org.apache.logging.log4j.LogManager
import org.slf4j.LoggerFactory

class App(player: Player, midiDeviceManager: MidiDeviceManager) extends AutoCloseable {
  def this(player: Player) = {
    this(player, MidiDeviceManager(player))
  }

  def this() = {
    this(Player())
  }

  override def close(): Unit = {
    midiDeviceManager.close()
  }
}

object App {
  private val log = LoggerFactory.getLogger(classOf[App.type])

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
      val app = new App()
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
}
