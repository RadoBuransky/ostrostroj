package com.buransky.ostrostroj.app.device

import java.util.concurrent.{Executors, TimeUnit}

import akka.actor.typed.ActorRef
import com.google.common.base.Preconditions._
import org.slf4j.LoggerFactory

import scala.collection.mutable

/**
 * Implementation using scheduled thread pool for dequeue timing.
 */
class SpiQueue(driver: ActorRef[DriverCommand], pins: Vector[GpioPin], periodMs: Int) extends AutoCloseable
  with Runnable {
  import SpiQueue._

  checkArgument(pins.nonEmpty)
  checkArgument(pins.length < 8)

  pins.zipWithIndex.foreach { case (p, i) =>
    logger.debug(s"Pin $i is ${p.pi4jPinAddress}.")
  }

  private val queue = mutable.Queue[Byte]()
  private val scheduler = Executors.newScheduledThreadPool(1)
  private val schedulerHandle = scheduler.scheduleAtFixedRate(this, periodMs, periodMs, TimeUnit.MILLISECONDS)
  logger.debug(s"Scheduler initialized. [$schedulerHandle]")

  override def close(): Unit = {
    logger.debug("Shutting down ScheduledEventsDequeue...")
    schedulerHandle.cancel(true)
    scheduler.shutdownNow()
    logger.info("ScheduledEventsDequeue shut down.")
  }

  override def run(): Unit = {
    try {
      queue.synchronized {
        if (queue.nonEmpty) {
          executePinStates(queue.dequeue())
        }
      }
    }
    catch {
      case t: Throwable =>
        logger.error("Scheduler crashed!", t);
        throw t
    }
  }

  def enqueue(pinStates: Iterable[Byte]): Unit = {
    val debugString = (0 to 2).map { bit =>
      pinStates.map { p =>
        ((p >> bit) & 1).toString
      }.mkString(pins(bit).pi4jPinAddress.toString + " - ", "", "")
    }.mkString(System.lineSeparator(), System.lineSeparator(), System.lineSeparator())
    logger.debug(debugString)

    queue.synchronized {
      queue.enqueueAll(pinStates)
    }
  }

  private def executePinStates(pinStates: Byte): Unit = {
    val sb = new mutable.StringBuilder()
    sb.append(pinStates)
    sb.append(" -> ")
    var shiftedPinStates: Int = pinStates
    pins.foreach { pin =>
      sb.append(pin.pi4jPinAddress);
      driver ! PinCommand(pin, (shiftedPinStates & 1) != 0)
//      if ((shiftedPinStates & 1) == 0) {
//        pin.low()
//        sb.append("l")
//      } else {
//        pin.high()
//        sb.append("h")
//      }
      sb.append(shiftedPinStates & 1)
      sb.append(" ")
      shiftedPinStates >>>= 1
    }
    logger.debug(sb.toString())
  }
}

private object SpiQueue {
  private val logger = LoggerFactory.getLogger(classOf[SpiQueue])
}