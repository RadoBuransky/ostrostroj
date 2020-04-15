package com.buransky.ostrostroj.app.device.max7219

import org.slf4j.LoggerFactory

import scala.collection.mutable

// Table 1. Serial-Data Format (16 Bits)
sealed abstract class RegisterAddress(val value: Byte)
sealed abstract class RegisterData(val value: Byte)

// Table 2. Register Address Map
case object NoOp extends RegisterAddress(0x00)
case object Digit0 extends RegisterAddress(0x01)
case object Digit1 extends RegisterAddress(0x02)
case object Digit2 extends RegisterAddress(0x03)
case object Digit3 extends RegisterAddress(0x04)
case object Digit4 extends RegisterAddress(0x05)
case object Digit5 extends RegisterAddress(0x06)
case object Digit6 extends RegisterAddress(0x07)
case object Digit7 extends RegisterAddress(0x08)
case object DecodeMode extends RegisterAddress(0x09)
case object Intensity extends RegisterAddress(0x0A)
case object ScanLimit extends RegisterAddress(0x0B)
case object Shutdown extends RegisterAddress(0x0C)
case object DisplayTest extends RegisterAddress(0x0F)

// 8-bit data byte.
final case class Data(v: Byte) extends RegisterData(v)

// Table 3. Shutdown Register Format (Address (Hex) = 0xXC)
case object ShutdownMode extends RegisterData(0x00)
case object ShutdownNormalOperation extends RegisterData(0x01)

// Table 4. Decode-Mode Register Examples (Address (Hex) = 0xX9)
case object NoDecode extends RegisterData(0x00)

// Table 7. Intensity Register Format (Address (Hex) = 0xXA)
case object IntensityMin extends RegisterData(0x00)
case object IntensityMid extends RegisterData(0x08)
case object IntensityMax extends RegisterData(0x0F)

// Table 8. Scan-Limit Register Format (Address (Hex) = 0xXB)
case object DisplayAllDigits extends RegisterData(0x07)

// Table 10. Display-Test Register Format (Address (Hex) = 0xXF)
case object TestNormalOperation extends RegisterData(0x00)
case object DisplayTestMode extends RegisterData(0x01)

// Cascaded MAX7219 chips from left (0) to right (3)
sealed abstract class Chip(val index: Int)
case object Chip0 extends Chip(0)
case object Chip1 extends Chip(1)
case object Chip2 extends Chip(2)
case object Chip3 extends Chip(3)

/**
 * Message to be sent to one of cascaded series of MAX7219 chips.
 */
final case class Message(address: RegisterAddress, data: RegisterData, chip: Chip)

/**
 * Configuration of Max7219Driver.
 *
 */
final case class Config(loadPin: (Boolean) => Any, clkPin: (Boolean) => Any, dinPin: (Boolean) => Any)

// Internal model
private[max7219] sealed trait Event
private[max7219] case object NoChange extends Event

private[max7219] sealed trait StateValue extends Event
private[max7219] case object High extends StateValue
private[max7219] case object Low extends StateValue

private[max7219] sealed trait StateChange extends Event
private[max7219] case object RaisingEdge extends StateChange
private[max7219] case object FallingEdge extends StateChange

private[max7219] final case class Events(load: Event, clk: Event, din: Event)

class Max7219Driver(config: Config) {
  import Max7219Driver._
  private val eventStream: mutable.Queue[Events] = new mutable.Queue[Events]()

  def send(msg: Message): Unit = {
    eventStream.synchronized {
      val events = MessageTranslator(msg)
      if (logger.isDebugEnabled) {
        logger.debug(s"Message [$msg] translated to ${events.length} events.")
      }
      eventStream.enqueueAll(events)
    }
  }

  def tick(): Unit = {
    eventStream.synchronized {
      if (eventStream.nonEmpty) {
        val events = eventStream.dequeue();
        executeEvent(events.load, config.loadPin)
        executeEvent(events.clk, config.clkPin)
        executeEvent(events.din, config.dinPin)
      }
    }
  }

  private def executeEvent(event: Event, pin: (Boolean) => Any): Unit = {
    event match {
      case High => pin(true)
      case Low => pin(false)
      case RaisingEdge => pin(true)
      case FallingEdge => pin(false)
      case NoChange =>
    }
  }
}

object Max7219Driver {
  private val logger = LoggerFactory.getLogger(classOf[Max7219Driver])
}