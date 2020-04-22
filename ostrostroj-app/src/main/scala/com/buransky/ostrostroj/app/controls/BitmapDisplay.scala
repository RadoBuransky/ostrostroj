package com.buransky.ostrostroj.app.controls

import akka.actor.typed.scaladsl.Behaviors
import akka.actor.typed.{ActorRef, Behavior}
import com.buransky.max7219.Max7219
import com.buransky.max7219.Max7219.PinState
import com.buransky.ostrostroj.app.common.OstrostrojException
import com.buransky.ostrostroj.app.device._

import scala.jdk.CollectionConverters._

object BitmapDisplay {
  sealed trait Config {
    def loadCsPin: GpioPin
    def clkPin: GpioPin
    def dinPin: GpioPin
    def displayRows: Int
    def displayColumns: Int
    def displaysVertically: Int
    def displaysHorizontally: Int
    def id: Int
    def periodNs: Int

    private[BitmapDisplay] def loadCsPinHigh: PinCommand
    private[BitmapDisplay] def loadCsPinLow: PinCommand
    private[BitmapDisplay] def clkPinHigh: PinCommand
    private[BitmapDisplay] def clkPinLow: PinCommand
    private[BitmapDisplay] def dinPinHigh: PinCommand
    private[BitmapDisplay] def dinPinLow: PinCommand
  }
  object Main extends Config {
    override val loadCsPin: GpioPin = Pin4
    override val clkPin: GpioPin = Pin3
    override val dinPin: GpioPin = Pin5
    override val displayRows: Int = 8
    override val displayColumns: Int = 8
    override val displaysVertically: Int = 1
    override val displaysHorizontally: Int = 4
    override val id: Int = 0
    override val periodNs: Int = 100

    private[BitmapDisplay] override val loadCsPinHigh = PinCommand(loadCsPin, state = true)
    private[BitmapDisplay] override val loadCsPinLow = PinCommand(loadCsPin, state = false)
    private[BitmapDisplay] override val clkPinHigh = PinCommand(clkPin, state = true)
    private[BitmapDisplay] override val clkPinLow = PinCommand(clkPin, state = false)
    private[BitmapDisplay] override val dinPinHigh = PinCommand(dinPin, state = true)
    private[BitmapDisplay] override val dinPinLow = PinCommand(dinPin, state = false)
  }

  /**
   * Draws bitmap.
   * @param bitmap pixels(x)(y)
   */
  final case class Draw(bitmap: Vector[Vector[Boolean]])

  def apply(driver: ActorRef[DriverCommand], config: Config): Behavior[Draw] = Behaviors.setup { ctx =>
    val ledMatrix = Max7219.initLedMatrix(config.displayRows, config.displayColumns, config.displaysVertically,
      config.displaysHorizontally)

    driver ! StartSpi(config.id, config.periodNs)
    driver ! enqueueLedMatrixResult(config, ledMatrix.reset())

    Behaviors.receiveMessage {
      case Draw(bitmap) =>
        for (column <- bitmap.indices) {
          for (row <- bitmap(column).indices) {
            ledMatrix.setLedStatus(row, column, bitmap(column)(row))
          }
        }
        driver ! enqueueLedMatrixResult(config, ledMatrix.draw())
        Behaviors.same
    }
  }

  private def enqueueLedMatrixResult(config: Config, result: java.lang.Iterable[PinState]): EnqueueToSpi = {
    val pinCommands = result.asScala.map {
      case PinState.LOADCS_HIGH => config.loadCsPinHigh;
      case PinState.LOADCS_LOW => config.loadCsPinLow;
      case PinState.CLK_HIGH => config.clkPinHigh;
      case PinState.CLK_LOW => config.clkPinLow;
      case PinState.DIN_HIGH => config.dinPinHigh;
      case PinState.DIN_LOW => config.dinPinLow;
      case other => throw new OstrostrojException(s"Unknown bit change! [$other]")
    }
    EnqueueToSpi(config.id, pinCommands)
  }
}
