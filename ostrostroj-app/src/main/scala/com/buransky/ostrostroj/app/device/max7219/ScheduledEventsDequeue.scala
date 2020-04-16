package com.buransky.ostrostroj.app.device.max7219

import java.util.concurrent.{Executors, TimeUnit}

import org.slf4j.LoggerFactory

/**
 * Implementation using scheduled thread pool for dequeue timing.
 */
class ScheduledEventsDequeue(executors: DequeueExecutors) extends AutoCloseable {
  import ScheduledEventsDequeue._

  private val eventsQueue = new EventsQueue()
  private val scheduler = Executors.newScheduledThreadPool(1)
  private val schedulerHandle = scheduler.scheduleAtFixedRate(() => command(), SCHEDULER_PERIOD_NS,
    SCHEDULER_PERIOD_NS, TimeUnit.NANOSECONDS)

  override def close(): Unit = {
    logger.debug("Shutting down ScheduledEventsDequeue...")
    schedulerHandle.cancel(true)
    scheduler.shutdownNow()
    logger.info("ScheduledEventsDequeue shut down.")
  }

  private def command(): Unit = {
    eventsQueue.dequeue(executors)
  }
}

private object ScheduledEventsDequeue {
  private val logger = LoggerFactory.getLogger(classOf[ScheduledEventsDequeue])

  /**
   * I'm assuming we'll never 100 ns
   */
  private val SCHEDULER_PERIOD_NS = 100
}