package com.buransky.ostrostroj.driver

trait GpioDriver extends AutoCloseable {
  def pins: Seq[GpioPin]
}

trait GpioPin {
  def setMode(mode: GpioPinMode): Unit
  def setValue(value: GpioPinValue): Unit
}

sealed trait GpioPinValue
object High extends GpioPinValue
object Low extends GpioPinValue

sealed trait GpioPinMode
object In extends GpioPinMode
object Out extends GpioPinMode