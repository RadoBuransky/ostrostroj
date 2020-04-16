package com.buransky.ostrostroj.app.device.max7219

import java.util.concurrent.{Executors, TimeUnit}

import org.slf4j.LoggerFactory

/**
 * Implementation using scheduled thread pool for dequeue timing.
 */
class ScheduledEventsDequeue(executors: DequeueExecutors) extends AutoCloseable with Runnable {
  import ScheduledEventsDequeue._

  private val eventsQueue = new EventsQueue()
  private val scheduler = Executors.newScheduledThreadPool(1)
  private val schedulerHandle = scheduler.scheduleAtFixedRate(this, SCHEDULER_PERIOD_MS,
    SCHEDULER_PERIOD_MS, TimeUnit.MILLISECONDS)
  logger.debug(s"Scheduler initialized. [$schedulerHandle]")

  override def close(): Unit = {
    logger.debug("Shutting down ScheduledEventsDequeue...")
    schedulerHandle.cancel(true)
    scheduler.shutdownNow()
    logger.info("ScheduledEventsDequeue shut down.")
  }

  override def run(): Unit = {
    logger.trace("Scheduled run.")
    eventsQueue.dequeue(executors)
  }
}

private object ScheduledEventsDequeue {
  private val logger = LoggerFactory.getLogger(classOf[ScheduledEventsDequeue])
  private val SCHEDULER_PERIOD_MS = 100
}