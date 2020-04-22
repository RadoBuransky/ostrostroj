package com.buransky.ostrostroj.app.device

import java.util.concurrent.{Executors, TimeUnit}

import org.slf4j.LoggerFactory

import scala.collection.mutable

/**
 * Implementation using scheduled thread pool for dequeue timing.
 */
class SpiQueue(commandExecutor: (PinCommand) => Unit, periodMs: Int) extends AutoCloseable
  with Runnable {
  import SpiQueue._

  private val queue = mutable.Queue[PinCommand]()
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
          commandExecutor(queue.dequeue())
        }
      }
    }
    catch {
      case t: Throwable =>
        logger.error("Scheduler crashed!", t);
        throw t
    }
  }

  def enqueue(pinStates: Iterable[PinCommand]): Unit = {
    queue.synchronized {
      queue.enqueueAll(pinStates)
    }
  }
}

private object SpiQueue {
  private val logger = LoggerFactory.getLogger(classOf[SpiQueue])
}