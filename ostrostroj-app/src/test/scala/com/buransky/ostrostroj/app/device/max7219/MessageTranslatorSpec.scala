package com.buransky.ostrostroj.app.device.max7219

import org.junit.runner.RunWith
import org.scalatest.flatspec.AnyFlatSpec
import org.scalatestplus.junit.JUnitRunner

import scala.annotation.tailrec

@RunWith(classOf[JUnitRunner])
class MessageTranslatorSpec extends AnyFlatSpec {
  behavior of "apply"

  it should "translate simple LED command" in {
    // Prepare
    val msg = Message(Digit0, Data(0x01), Chip0)

    // Execute
    val result = MessageTranslator(msg)

    // Debugging
    println(printTranslation(msg, result))

    // Assert
    assertLoad(result.map(_.load))
    assertClk(result.map(_.clk))
    assertDin(result.map(_.din))

    val dataBits = extractAllDataBits(result)
    assert(dataBits == Integer.parseInt("000000100000001", 2), "Extracted bits = " + Integer.toBinaryString(dataBits))
    assertTotalDataBits(result)
  }

  private def assertLoad(loadEvents: Seq[Event]): Unit = {
  }

  private def assertClk(clkEvents: Seq[Event]): Unit = {
  }

  private def assertDin(dinEvents: Seq[Event]): Unit = {
  }

  private def assertDontCareBits(events: Seq[Events]): Unit = {
  }

  private def assertOneFallingLoadEdge(events: Seq[Events]): Unit = {
  }

  private def assertOneRisingLoadEdge(events: Seq[Events]): Unit = {
  }

  private def assertLoadEdgesOrder(events: Seq[Events]): Unit = {
  }

  /**
   * Assert that total number of rising CLK edges between falling and rising LOAD edges is 16.
   */
  private def assertTotalDataBits(events: Seq[Events]): Unit = {
    var loadLow = false
    var count = 0
    events.foreach { e =>
      if (loadLow) {
        if (e.load == RaisingEdge) {
          loadLow = false
        } else {
          if (e.clk == RaisingEdge) {
            count += 1
          }
        }
      } else {
        if (e.load == FallingEdge) {
          loadLow = true
        }
      }
    }

    assert(count == MessageTranslator.CHIP_COUNT*16)
  }

  private def getBits(num: Int, from: Int, to: Int): Int = (num << (31 - to)) >>> (from + (31 - to))
  private def getBit(num: Int, i: Int): Int = if ((num & (1 << i)) != 0) 1 else 0

  private def extractAllDataBits(events: List[Events]): Int = {
    @tailrec
    def rec(events: List[Events], lastDin: Option[Int], acc: Int): Int = {
      events match {
        case h :: t if h.clk == RaisingEdge =>
          lastDin match {
            case Some(value) => rec(t, lastDin, (acc << 1) | value)
            case None => fail("No previous DIN!")
          }
        case h :: t if h.din == High => rec(t, Some(1), acc)
        case h :: t if h.din == Low => rec(t, Some(0), acc)
        case _ :: t => rec(t, lastDin, acc)
        case Nil => acc
      }
    }
    rec(events, None, 0)
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
