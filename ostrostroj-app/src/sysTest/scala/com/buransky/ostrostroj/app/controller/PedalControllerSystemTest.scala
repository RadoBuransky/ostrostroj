package com.buransky.ostrostroj.app.controller

import akka.actor.testkit.typed.Effect.Spawned
import akka.actor.testkit.typed.scaladsl.{BehaviorTestKit, TestProbe}
import akka.actor.typed.Behavior
import akka.actor.typed.scaladsl.Behaviors
import com.buransky.ostrostroj.app.controller.Model.{LedBtn1, LedColor}
import com.buransky.ostrostroj.app.controller.PedalController.{ControllerCommand, LedControllerCommand}
import com.buransky.ostrostroj.app.controller.hw.Gpio.{Pin0, Pin1, Pin2}
import com.buransky.ostrostroj.app.controller.hw._
import com.buransky.ostrostroj.app.controller.hw.part.RgbLed
import com.buransky.ostrostroj.app.sysTest.BaseSystemTest

class RealPedalControllerSystemTest extends PedalControllerSystemTestBase(false) {
  behavior of "An real Ostrostroj controller (not emulated)"

  it should "be able to start" in {
    val behaviorTestKit = BehaviorTestKit(PedalController(PedalController.Params(false), () => EmulatorDriver(),
      () => OdroidC2Driver(), (driver, config) => RgbLed(driver, config)))
    behaviorTestKit.expectEffect(Spawned(OdroidC2Driver(), "driver"))
  }

  createTests()
}

class EmulatedPedalControllerSystemTest extends PedalControllerSystemTestBase(true) {
  behavior of "An Ostrostroj controller using an emulator of Odroid C2"

  it should "be able to start" in {
    val behaviorTestKit = BehaviorTestKit(PedalController(PedalController.Params(true), () => EmulatorDriver(),
      () => OdroidC2Driver(), (driver, config) => RgbLed(driver, config)))
    behaviorTestKit.expectEffect(Spawned(EmulatorDriver(), "driver"))
  }

  createTests()
}

 abstract class PedalControllerSystemTestBase(useEmulator: Boolean) extends BaseSystemTest {
   private val monitoringProbe: TestProbe[PinCommand] = testKit.createTestProbe[PinCommand]()

   def createTests(): Unit = {
     it should "be able to make led1 red" in {
       val pedalController = testKit.spawn(createBehavior())
       pedalController ! LedControllerCommand(LedBtn1.led, LedColor(r = true, g = false, b = false))
       monitoringProbe.expectMessage(PinCommand(Pin0, PinHigh))
       monitoringProbe.expectMessage(PinCommand(Pin1, PinLow))
       monitoringProbe.expectMessage(PinCommand(Pin2, PinLow))
     }

     it should "be able to make led1 green" in {
       val pedalController = testKit.spawn(createBehavior())
       pedalController ! LedControllerCommand(LedBtn1.led, LedColor(r = false, g = true, b = false))
       monitoringProbe.expectMessage(PinCommand(Pin0, PinLow))
       monitoringProbe.expectMessage(PinCommand(Pin1, PinHigh))
       monitoringProbe.expectMessage(PinCommand(Pin2, PinLow))
     }

     it should "be able to make led1 blue" in {
       val pedalController = testKit.spawn(createBehavior())
       pedalController ! LedControllerCommand(LedBtn1.led, LedColor(r = false, g = false, b = true))
       monitoringProbe.expectMessage(PinCommand(Pin0, PinLow))
       monitoringProbe.expectMessage(PinCommand(Pin1, PinLow))
       monitoringProbe.expectMessage(PinCommand(Pin2, PinHigh))
     }
   }

   protected def createBehavior(): Behavior[ControllerCommand] = PedalController(PedalController.Params(useEmulator),
     () => Behaviors.monitor(monitoringProbe.ref, EmulatorDriver()),
     () => Behaviors.monitor(monitoringProbe.ref, OdroidC2Driver()),
     (driver, config) => RgbLed(driver, config))
 }