package com.buransky.ostrostroj.app.controls

import akka.NotUsed
import akka.actor.typed.scaladsl.{AbstractBehavior, ActorContext, Behaviors}
import akka.actor.typed.{ActorRef, Behavior, SupervisorStrategy}
import com.buransky.ostrostroj.app.common.OstrostrojConfig
import com.buransky.ostrostroj.app.controls.RgbLed.Led1
import com.buransky.ostrostroj.app.controls.display.BitmapDisplay
import com.buransky.ostrostroj.app.controls.display.BitmapDisplay.{Position, Repaint, Write}
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
    private val bitmapDisplay = ctx.spawn(Behaviors.supervise(BitmapDisplay(driver, BitmapDisplay.Main))
      .onFailure(SupervisorStrategy.restart), "display")
    bitmapDisplay ! Write("Ostrostroj", Position(0, 0), color = true)
    bitmapDisplay ! Repaint

    override def onMessage(msg: NotUsed): Behavior[NotUsed] = Behaviors.ignore

    if (OstrostrojConfig.develeoperMode) {
      ctx.spawn(Keyboard(driver, bitmapDisplay), "keyboard")
    }
  }
}