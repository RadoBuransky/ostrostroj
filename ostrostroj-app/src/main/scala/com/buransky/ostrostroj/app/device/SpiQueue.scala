package com.buransky.ostrostroj.app.device

import java.util.concurrent.{Executors, TimeUnit}

import com.google.common.base.Preconditions._
import com.pi4j.io.gpio.GpioPinDigitalOutput
import org.slf4j.LoggerFactory

import scala.collection.mutable

/**
 * Implementation using scheduled thread pool for dequeue timing.
 */
class SpiQueue(pins: Vector[GpioPinDigitalOutput], periodNs: Int) extends AutoCloseable with Runnable {
  import SpiQueue._

  checkArgument(pins.nonEmpty)
  checkArgument(pins.length < 8)

  private val queue = mutable.Queue[Byte]()
  private val scheduler = Executors.newScheduledThreadPool(1)
  private val schedulerHandle = scheduler.scheduleAtFixedRate(this, periodNs, periodNs, TimeUnit.NANOSECONDS)
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
    queue.synchronized {
      queue.enqueueAll(pinStates)
      logger.debug(s"Pin states enqueued. [${pinStates.size}]");
    }
  }

  private def executePinStates(pinStates: Byte): Unit = {
    logger.debug("Execute pin states = " + Integer.toBinaryString(pinStates))
    var shiftedPinStates: Int = pinStates
    for (i <- 0 to pins.length) {
      val pin = pins(i)
      if ((shiftedPinStates & 1) == 0) {
        pin.low()
      } else {
        pin.high()
      }
      shiftedPinStates >>>= 1
    }
  }
}

private object SpiQueue {
  private val logger = LoggerFactory.getLogger(classOf[SpiQueue])
}