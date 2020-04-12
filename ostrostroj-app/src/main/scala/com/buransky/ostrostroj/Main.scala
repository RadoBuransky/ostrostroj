package com.buransky.ostrostroj

import com.pi4j.io.gpio.{GpioFactory, OdroidC1Pin, PinState}
import com.pi4j.platform.{Platform, PlatformManager}
import javax.sound.sampled.{AudioFormat, AudioSystem, Clip, DataLine}
import org.slf4j.LoggerFactory

/**
 * Ostrostroj App entry point.
 */
object Main {
  private val logger = LoggerFactory.getLogger(this.getClass)

  def main(args: Array[String]): Unit = {
    try {
      soundPoc()
    }
    catch {
      case ex: Throwable =>
        logger.error("Ostrostroj crashed!", ex)
        throw ex
    }
  }

  private def soundPoc(): Unit = {
    val format = new AudioFormat(44100, 16, 2, true, true)

    val mixerInfos = AudioSystem.getMixerInfo
    mixerInfos.foreach { mi =>
      logger.info(s"Mixer info = $mi")
      val mixer = AudioSystem.getMixer(mi)

      val sourceLineInfos = mixer.getSourceLineInfo
      sourceLineInfos.foreach(sli => logger.info(s"  Source line = $sli"))

      val targetLineInfos = mixer.getTargetLineInfo
      targetLineInfos.foreach(tli => logger.info(s"  Target line = $tli"))

      try {
        val sourceLine1 = AudioSystem.getSourceDataLine(format, mi)
        val sourceLine2 = AudioSystem.getSourceDataLine(format, mi)
        val syncSupported = mixer.isSynchronizationSupported(Array(sourceLine1, sourceLine2), false)
        logger.info(s"Sync supported = $syncSupported")
      }
      catch {
        case _: Throwable =>
      }
    }

    val dataLineInfo = new DataLine.Info(classOf[Clip],  format)
    logger.info(s"Data line = $dataLineInfo")

    val mixerInfo = mixerInfos(1)
    val sourceLine1 = AudioSystem.getSourceDataLine(format, mixerInfo)
    logger.info(s"Source line 2 = $sourceLine1")

    val sourceLine2 = AudioSystem.getSourceDataLine(format, mixerInfo)
    logger.info(s"Source line 2 = $sourceLine2")

    val mixer = AudioSystem.getMixer(mixerInfo)
    logger.info(s"Mixer = $mixer")
  }

  private def rgbLedPoc(): Unit = {
    PlatformManager.setPlatform(Platform.ODROID)
    val gpio = GpioFactory.getInstance()
    val pin0 = gpio.provisionDigitalOutputPin(OdroidC1Pin.GPIO_00, "B", PinState.LOW)
    val pin1 = gpio.provisionDigitalOutputPin(OdroidC1Pin.GPIO_01, "R", PinState.LOW)
    val pin2 = gpio.provisionDigitalOutputPin(OdroidC1Pin.GPIO_02, "G", PinState.LOW)
    pin0.setShutdownOptions(false, PinState.LOW)
    pin1.setShutdownOptions(false, PinState.LOW)
    pin2.setShutdownOptions(false, PinState.LOW)
    pin0.high() // Blue
    Thread.sleep(1000)
    pin0.low()
    pin1.high() // Red
    Thread.sleep(1000)
    pin1.low()
    pin2.high() // Green
    Thread.sleep(1000)
    pin1.high() // Yellow
    Thread.sleep(1000)
    pin2.low()
    pin0.high() // Cyan
    Thread.sleep(1000)
    pin1.low()
    pin2.high() // Magenta
    Thread.sleep(1000)
    pin0.high()
    pin1.high() // White
    Thread.sleep(1000)
  }
}
