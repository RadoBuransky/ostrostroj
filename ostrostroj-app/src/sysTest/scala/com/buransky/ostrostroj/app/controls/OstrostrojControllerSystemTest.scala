package com.buransky.ostrostroj.app.controls

import akka.actor.testkit.typed.Effect.Spawned
import akka.actor.testkit.typed.scaladsl.BehaviorTestKit
import com.buransky.ostrostroj.app.device._
import com.buransky.ostrostroj.app.sysTest.BaseSystemTest

class OstrostrojControllerSystemTest extends BaseSystemTest {
  behavior of "An real Ostrostroj controller (not emulated)"

  it should "be able to start" in {
    val driverProbe = testKit.createTestProbe[DriverCommand]()
    val behaviorTestKit = BehaviorTestKit(OstrostrojController(driverProbe.ref))
    behaviorTestKit.expectEffect(Spawned(OdroidGpio(), "driver"))
  }
}