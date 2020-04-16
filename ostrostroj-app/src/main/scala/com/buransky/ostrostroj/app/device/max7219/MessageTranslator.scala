package com.buransky.ostrostroj.app.device.max7219

/**
 * Translates single MAX7219 input message to sequence of events.
 */
object MessageTranslator {
  /**
   * Most-significant bit (MSB) is first in the list (head).
   */
  def apply(msg: Message): List[Events] =
    introEvents() ::: dontCareEvents() ::: addressEvents(msg.address) ::: dataEvents(msg.data) ::: chipEvents(msg.chip) ::: outroEvents()

  private def introEvents(): List[Events] = List(
    Events(load = High,         clk = Low,        din = Low),
    Events(load = FallingEdge,  clk = NoChange,   din = NoChange)
  )

  private def dontCareEvents(): List[Events] =
    bitEvents(false) ::: bitEvents(false) ::: bitEvents(false) ::: bitEvents(false)

  private def addressEvents(address: RegisterAddress): List[Events] = byteEvents(address.value, 4)
  private def dataEvents(data: RegisterData): List[Events] = byteEvents(data.value, 8)

  /**
   * Nah, don't you think we should be able to do better here?
   */
  private def chipEvents(chip: Chip): List[Events] = {
    val result = for {
      i <- 1 to chip.index
    } yield byteEvents(0, 16)
    result.flatten.toList
  }
  private def outroEvents(): List[Events] = List(
    Events(load = NoChange,    clk = Low,      din = NoChange),
    Events(load = RaisingEdge, clk = NoChange, din = NoChange)
  )

  private def byteEvents(value: Byte, bits: Int): List[Events] = {
    val result = for {
      i <- (bits - 1) to 0 by -1
    } yield bitEvents(getBit(value, i))
    result.flatten.toList
  }

  private def bitEvents(bit: Boolean): List[Events] = {
    val dinStateValue = if (bit) High else Low
    List (
      Events(load = NoChange, clk = NoChange,    din = dinStateValue),
      Events(load = NoChange, clk = RaisingEdge, din = NoChange),
      Events(load = NoChange, clk = FallingEdge, din = NoChange)
    )
  }

  private def getBit(num: Int, i: Int): Boolean = ((num & (1 << i)) != 0)
}
