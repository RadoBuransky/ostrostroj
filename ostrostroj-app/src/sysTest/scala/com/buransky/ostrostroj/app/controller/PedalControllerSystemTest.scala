package com.buransky.ostrostroj.app.controller

import org.scalatest.flatspec.AnyFlatSpec

class PedalControllerSystemTest extends AnyFlatSpec {
  behavior of "An real Ostrostroj controller (not emulated)"

  it should "be able to start" in {
    PedalController(PedalController.Params(useEmulator = false))
  }

  it should "fail" in {
    ???
  }
}
