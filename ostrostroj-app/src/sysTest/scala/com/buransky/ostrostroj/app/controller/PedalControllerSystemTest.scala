package com.buransky.ostrostroj.app.controller

import akka.actor.testkit.typed.Effect.Spawned
import akka.actor.testkit.typed.scaladsl.{BehaviorTestKit, TestProbe}
import akka.actor.typed.Behavior
import com.buransky.ostrostroj.app.controller.Model.{LedBtn1, LedColor}
import com.buransky.ostrostroj.app.controller.PedalController.{ControllerCommand, LedControllerCommand}
import com.buransky.ostrostroj.app.device.{DriverCommand, OdroidC2Driver, Pin0, Pin1, Pin2, PinCommand}
import com.buransky.ostrostroj.app.sysTest.BaseSystemTest

class RealPedalControllerSystemTest extends PedalControllerSystemTestBase(false) {
  behavior of "An real Ostrostroj controller (not emulated)"

  it should "be able to start" in {
    val driverProbe = testKit.createTestProbe[DriverCommand]()
    val behaviorTestKit = BehaviorTestKit(PedalController(driverProbe.ref))
    behaviorTestKit.expectEffect(Spawned(OdroidC2Driver(), "driver"))
  }

  createTests()
}

 abstract class PedalControllerSystemTestBase(useEmulator: Boolean) extends BaseSystemTest {
   private val monitoringProbe: TestProbe[PinCommand] = testKit.createTestProbe[PinCommand]()

   def createTests(): Unit = {
     it should "be able to make led1 red" in {
       val pedalController = testKit.spawn(createBehavior())
       pedalController ! LedControllerCommand(LedBtn1.led, LedColor(r = true, g = false, b = false))
       monitoringProbe.expectMessage(PinCommand(Pin0, true))
       monitoringProbe.expectMessage(PinCommand(Pin1, false))
       monitoringProbe.expectMessage(PinCommand(Pin2, false))
     }

     it should "be able to make led1 green" in {
       val pedalController = testKit.spawn(createBehavior())
       pedalController ! LedControllerCommand(LedBtn1.led, LedColor(r = false, g = true, b = false))
       monitoringProbe.expectMessage(PinCommand(Pin0, false))
       monitoringProbe.expectMessage(PinCommand(Pin1, true))
       monitoringProbe.expectMessage(PinCommand(Pin2, false))
     }

     it should "be able to make led1 blue" in {
       val pedalController = testKit.spawn(createBehavior())
       pedalController ! LedControllerCommand(LedBtn1.led, LedColor(r = false, g = false, b = true))
       monitoringProbe.expectMessage(PinCommand(Pin0, false))
       monitoringProbe.expectMessage(PinCommand(Pin1, false))
       monitoringProbe.expectMessage(PinCommand(Pin2, true))
     }
   }

   protected def createBehavior(): Behavior[ControllerCommand] = {
     val driverProbe = testKit.createTestProbe[DriverCommand]()
     PedalController(driverProbe.ref)
   }
 }