package com.buransky.ostrostroj.driver

private[driver] class MmapGpioPin(val num: Int) extends GpioPin {
  import MmapGpioPin._

  private val reg = if (num < C2_GPIOX_PIN_START) C2_GPIOY_FSEL_REG_OFFSET else C2_GPIOX_FSEL_REG_OFFSET
  private val shift = C2_GP_TO_SHIFT_REG(num - C2_GPIOY_PIN_START)

  override def setMode(mode: GpioPinMode): Unit = {
  }
  override def setValue(value: GpioPinValue): Unit = ???
}

private object MmapGpioPin {
  private val C2_GPIO_PIN_BASE = 0x88
  private val C2_GPIOY_PIN_START = C2_GPIO_PIN_BASE + 0x4b
  private val C2_GPIOY_PIN_END = C2_GPIO_PIN_BASE + 0x5b
  private val C2_GPIOX_PIN_START = C2_GPIO_PIN_BASE + 0x5c
  private val C2_GPIOX_PIN_END = C2_GPIO_PIN_BASE + 0x72
  private val C2_GPIOX_FSEL_REG_OFFSET = 0x118
  private val C2_GPIOY_FSEL_REG_OFFSET = 0x10F
  private val C2_GP_TO_SHIFT_REG = IndexedSeq(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22)
}