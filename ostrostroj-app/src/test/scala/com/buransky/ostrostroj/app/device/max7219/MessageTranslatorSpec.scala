package com.buransky.ostrostroj.app.device.max7219

import org.junit.runner.RunWith
import org.scalatest.flatspec.AnyFlatSpec
import org.scalatestplus.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
class MessageTranslatorSpec extends AnyFlatSpec {
  behavior of "apply"

  it should "translate shutdown message" in {
    // Prepare
    val msg = Message(Digit3, Data(0xFF.toByte), Chip3)

    // Execute
    val result = MessageTranslator(msg)

    // Debugging
    println(printTranslation(msg, result))

    // Assert
    assert(result.length == 196)
  }

  private def printTranslation(msg: Message, events: List[Events]): String = {
    val sb = new StringBuilder()
    val ln = System.lineSeparator()
    sb.append(s"message = $msg")
    sb.append(ln)
    val printedEvents = printEventsStream(events)
    printedEvents.transpose.map(_.mkString).foreach { l =>
      sb.append(l)
      sb.append(ln)
    }
    sb.toString()
  }

  private def printEventsStream(events: List[Events]): List[List[Char]] = {
    events match {
      case h :: t => printEvents(h) :: printEventsStream(t)
      case Nil => Nil
    }
  }

  private def printEvents(events: Events): List[Char] =
    List(printEvent(events.load), printEvent(events.clk), printEvent(events.din))

  private def printEvent(event: Event): Char = event match {
    case NoChange => '.'
    case High => '‾'
    case Low => '_'
    case RaisingEdge => '/'
    case FallingEdge => '\\'
  }
}
