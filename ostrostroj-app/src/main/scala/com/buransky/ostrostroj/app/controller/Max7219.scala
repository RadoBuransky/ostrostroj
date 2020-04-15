package com.buransky.ostrostroj.app.controller

import akka.actor.typed.scaladsl.{AbstractBehavior, ActorContext, Behaviors, TimerScheduler}
import akka.actor.typed.{ActorRef, Behavior}
import com.buransky.ostrostroj.app.device.Gpio.GpioPin
import com.buransky.ostrostroj.app.device.PinCommand
import org.slf4j.LoggerFactory

import scala.collection.mutable
import scala.concurrent.duration._

/**
 * MAX7219 chip driver. From the official specification CLK max 10MHz means at least 100 nanoseconds period length, also
 * CLK low and high width must be at least 50 nanoseconds.
 */
object Max7219 {
  private val logger = LoggerFactory.getLogger(Max7219.getClass)
  final case class Config(dinPin: GpioPin, csPin: GpioPin, clkPin: GpioPin)
  sealed trait Command
  /**
   * See Table 1. Serial-Data Format (16 Bits) in MAX7219 specification document.
   * @param address 4-bit address (use least-significant bits)
   * @param data 8-bit data
   * @param chipIndex Index of MAX7219 in case of cascaded design (0-based).
   */
  case class Word(address: Byte, data: Byte, chipIndex: Int) extends Command
  private case object ClkTimeout extends Command

//  private val CLK_PERIOD = 100.nanoseconds
  private val CLK_PERIOD = 1.millisecond
  private case object ClkTimerKey

  def apply(driver: ActorRef[PinCommand], config: Config): Behavior[Max7219.Command] = Behaviors.setup { ctx =>
    Behaviors.withTimers { timers =>
      new Max7219Behavior(driver, config, timers, ctx)
    }
  }

  class Max7219Behavior(driver: ActorRef[PinCommand],
                        config: Config,
                        timers: TimerScheduler[Max7219.Command],
                        ctx: ActorContext[Max7219.Command]) extends AbstractBehavior[Max7219.Command](ctx) {
    private val words = new mutable.Queue[Word]()
    private val bits = new mutable.Queue[Boolean]()
    private var clkState: Boolean = false

    override def onMessage(msg: Max7219.Command): Behavior[Max7219.Command] = {
      msg match {
        case word: Word =>
          val wasEmpty = words.isEmpty
          words.enqueue(word)
          if (wasEmpty) {
            if (!timers.isTimerActive(ClkTimerKey)) {
              // Init
              driver ! PinCommand(config.csPin, true)
              logger.debug(s"CS = true")
              driver ! PinCommand(config.clkPin, false)
              logger.debug(s"CLK = false")
              driver ! PinCommand(config.dinPin, false)
              logger.debug(s"DIN = false")

              timers.startTimerAtFixedRate(ClkTimerKey, ClkTimeout, CLK_PERIOD)
              logger.debug(s"CLK timer started. [$CLK_PERIOD]")
            }
          }
          Behaviors.same

        case ClkTimeout =>
          var lastBit = false

          if (!clkState) {
            if (bits.nonEmpty) {
              val bit = bits.dequeue()
              driver ! PinCommand(config.dinPin, bit)
              logger.debug(s"DIN = $bit")

              if (bits.isEmpty) {
                lastBit = true
              }
            }
            else {
              if (words.nonEmpty) {
                val word = words.dequeue()
                logger.debug(s"Word dequeued. [${word.address}, ${word.data}]")
                enqueueWordBits(word)
                driver ! PinCommand(config.csPin, false)
                logger.debug(s"CS = false")
              }
              else {
                timers.cancel(ClkTimerKey)
                logger.debug(s"CLK timer stopped.")
              }
            }
          }

          // Toggle CLK state
          clkState = !clkState
          driver ! PinCommand(config.clkPin, clkState)
          logger.debug(s"CLK = $clkState")

          if (lastBit) {
            driver ! PinCommand(config.csPin, true)
            logger.debug(s"CS = true")
          }

          Behaviors.same
      }
    }

    private def enqueueWordBits(word: Word): Unit = {
      logger.debug(s"Enqueuing word... [${word.address}, ${word.data}]")

      // D15 - D12 are ignored
      enqueueBit(false)
      enqueueBit(false)
      enqueueBit(false)
      enqueueBit(false)

      // D11 - D8 are address bits
      enqueueBit(getBit(word.address, 3))
      enqueueBit(getBit(word.address, 2))
      enqueueBit(getBit(word.address, 1))
      enqueueBit(getBit(word.address, 0))

      // D7 - D0 are data bits
      enqueueBit(getBit(word.data, 7))
      enqueueBit(getBit(word.data, 6))
      enqueueBit(getBit(word.data, 5))
      enqueueBit(getBit(word.data, 4))
      enqueueBit(getBit(word.data, 3))
      enqueueBit(getBit(word.data, 2))
      enqueueBit(getBit(word.data, 1))
      enqueueBit(getBit(word.data, 0))

      for (i <- 1 to word.chipIndex) {
        enqueueNoOp()
      }

      logger.debug(s"Word enqueued.")
    }

    private def enqueueNoOp(): Unit = {
      for (i <- 0 to 15) {
        enqueueBit(false)
      }
    }

    private def enqueueBit(bit: Boolean): Unit = {
      bits.enqueue(bit)
      logger.debug(s"Bit enqueued. [$bit]")
    }

    private def getBit(num: Int, i: Int): Boolean = ((num & (1 << i)) != 0)
  }
}
