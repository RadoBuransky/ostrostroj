package com.buransky.ostrostroj.app.controller

/**
 * Logical model of the controller.
 */
object Model {
  /**
   * Main LED display matrix 32x8
   */
  object Display

  /**
   * Left top button.
   */
  object ControlBtn

  /**
   * Right top button.
   */
  object LoopBtn

  /**
   * Four groups of LED and button on the bottom.
   */
  final case object LedBtn1 extends LedBtnGroup
  final case object LedBtn2 extends LedBtnGroup
  final case object LedBtn3 extends LedBtnGroup
  final case object LedBtn4 extends LedBtnGroup

  /**
   * Logical LED.
   */
  class Led
  final case class LedColor(r: Boolean, g: Boolean, b: Boolean)

  /**
   * Logical button.
   */
  class Btn
  sealed trait BtnEvent
  final case object BtnDown extends BtnEvent
  final case object BtnUp extends BtnEvent

  /**
   * Logical LED button group.
   */
  sealed trait LedBtnGroup {
    val led = new Led()
    val btn = new Btn()
  }
}
