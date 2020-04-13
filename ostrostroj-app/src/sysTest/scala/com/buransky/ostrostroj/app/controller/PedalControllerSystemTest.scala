package com.buransky.ostrostroj.app.controller

import akka.actor.testkit.typed.Effect.Spawned
import akka.actor.testkit.typed.scaladsl.BehaviorTestKit
import com.buransky.ostrostroj.app.controller.hw.OdroidC2Driver
import com.buransky.ostrostroj.app.sysTest.BaseSystemTest

class PedalControllerSystemTest extends BaseSystemTest {
  behavior of "An real Ostrostroj controller (not emulated)"

  it should "be able to start" in {
    val testKit = BehaviorTestKit(PedalController(PedalController.Params(useEmulator = false)))
    testKit.expectEffect(Spawned(OdroidC2Driver(), "odroid"))
  }
}
