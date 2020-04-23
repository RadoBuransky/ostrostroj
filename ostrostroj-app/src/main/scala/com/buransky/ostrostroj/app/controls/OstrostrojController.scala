package com.buransky.ostrostroj.app.controls

import akka.NotUsed
import akka.actor.typed.scaladsl.{AbstractBehavior, ActorContext, Behaviors}
import akka.actor.typed.{ActorRef, Behavior}
import com.buransky.ostrostroj.app.common.OstrostrojConfig
import com.buransky.ostrostroj.app.controls.RgbLed.Led1
import com.buransky.ostrostroj.app.controls.display.BitmapDisplay
import com.buransky.ostrostroj.app.device._

/**
 * Orchestrator for physical controls.
 */
object OstrostrojController {

  def apply(driver: ActorRef[DriverCommand]): Behavior[NotUsed] = Behaviors.setup { ctx =>
    new PedalControllerBehavior(driver, ctx)
  }

  class PedalControllerBehavior(driver: ActorRef[DriverCommand],
                                ctx: ActorContext[NotUsed]) extends AbstractBehavior[NotUsed](ctx) {
    private val led1 = ctx.spawn(RgbLed(driver, Led1), "led1")
    private val bitmapDisplay = ctx.spawn(BitmapDisplay(driver, BitmapDisplay.Main), "display")

    override def onMessage(msg: NotUsed): Behavior[NotUsed] = Behaviors.ignore

    if (OstrostrojConfig.develeoperMode) {
      ctx.spawn(Keyboard(driver, bitmapDisplay), "keyboard")
    }
  }
}