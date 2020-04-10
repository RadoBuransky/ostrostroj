package com.buransky.ostrostroj

import com.buransky.ostrostroj.driver.{High, Low, MmapGpioDriver}
import org.slf4j.LoggerFactory

/**
 * Ostrostroj App entry point.
 */
object Main {
  private val logger = LoggerFactory.getLogger(this.getClass)

  def main(args: Array[String]): Unit = {
    try {
      val gpioDriver = MmapGpioDriver()
      try {
        gpioDriver.pins(211).setValue(Low)
        gpioDriver.pins(212).setValue(Low)
        gpioDriver.pins(213).setValue(Low)

        gpioDriver.pins(211).setValue(High)
        Thread.sleep(1000)
        gpioDriver.pins(212).setValue(High)
        Thread.sleep(1000)
        gpioDriver.pins(213).setValue(High)
        Thread.sleep(1000)
      }
      finally {
        gpioDriver.close()
      }
    }
    catch {
      case ex: Throwable =>
        logger.error("Ostrostroj crashed!", ex)
        throw ex
    }
  }
}
