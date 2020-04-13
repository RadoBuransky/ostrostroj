package com.buransky.ostrostroj.app.sysTest

import akka.actor.testkit.typed.scaladsl.ScalaTestWithActorTestKit
import org.scalatest.flatspec.AnyFlatSpecLike

abstract class BaseSystemTest extends ScalaTestWithActorTestKit with AnyFlatSpecLike {
  // Static initialization
  BaseSystemTest
}

object BaseSystemTest {
  import org.slf4j.bridge.SLF4JBridgeHandler
  SLF4JBridgeHandler.removeHandlersForRootLogger()
  SLF4JBridgeHandler.install()
}