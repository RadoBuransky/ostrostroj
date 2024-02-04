package com.buransky.ostrostroj.player.midi

import com.buransky.ostrostroj.player.MidiMessageOps
import com.buransky.ostrostroj.player.midi.MidiReceiver.{log, nanosPerMicro}
import org.slf4j.LoggerFactory

import java.time.Duration
import java.util.HexFormat
import java.util.concurrent.TimeUnit
import javax.sound.midi._
import scala.util.control.NonFatal

class MidiReceiver(device: MidiDevice, commands: MidiCommands) extends Receiver {
  override def send(message: MidiMessage, timestampMicros: Long): Unit = {
    try {
      val timestamp = instantFromMicros(timestampMicros)
      message.command match {
        case ShortMessage.TIMING_CLOCK =>
          commands.tick()
        case ShortMessage.START =>
          commands.start()
        case ShortMessage.CONTINUE =>
          commands.continue()
        case ShortMessage.STOP =>
          commands.stop()
        case ShortMessage.PROGRAM_CHANGE =>
          commands.programChange(message.asInstanceOf[ShortMessage].getData1)
        case _ =>
      }
      if (log.isDebugEnabled) {
        debugLog(message, timestamp)
      }
    } catch {
      case NonFatal(ex) =>
        log.error(s"Receiver failed! " +
          s"${message.getClass.getSimpleName}(${HexFormat.of().formatHex(message.getMessage)})", ex)
    }
  }

  override def close(): Unit = {
    device.close()
    log.info(s"${device.getDeviceInfo.getName} closed.")
  }

  private def instantFromMicros(micros: Long): Duration = {
    Duration.ofNanos(Math.multiplyExact(micros, nanosPerMicro))
  }

  private def debugLog(message: MidiMessage, timestamp: Duration): Unit = {
    message match {
      case sm: ShortMessage =>
        if (message.command == ShortMessage.TIMING_CLOCK) {
          log.trace(s"$timestamp ShortMessage: TIMING_CLOCK ${sm.getData1} ${sm.getData2}")
        } else {
          message.channel match {
            case Some(channel) =>
              log.debug(s"$timestamp ShortMessage: ch$channel ${message.command} ${sm.getData1} ${sm.getData2}")
            case None =>
              log.debug(s"$timestamp ShortMessage: ${message.command} ${sm.getData1} ${sm.getData2}")
          }
        }
      case sm: SysexMessage =>
        log.debug(s"$timestamp SysexMessage: ${sm.getStatus} ${HexFormat.of().formatHex(sm.getData)}")
      case mm: MetaMessage =>
        mm.getData
        log.debug(s"$timestamp MetaMessage: ${mm.getStatus} ${mm.getType} ${HexFormat.of().formatHex(mm.getData)}")
      case _ =>
        log.debug(s"$timestamp ${message.getClass.getSimpleName}: ${HexFormat.of().formatHex(message.getMessage)}")
    }
  }
}

object MidiReceiver {
  private val log = LoggerFactory.getLogger(classOf[MidiReceiver])
  private val nanosPerMicro = TimeUnit.MICROSECONDS.toNanos(1L)
}