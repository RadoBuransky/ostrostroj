package com.buransky.ostrostroj.app.audio.impl.input

import com.buransky.ostrostroj.app.audio.{AudioBuffer, AudioEvent, LoopInput}
import com.buransky.ostrostroj.app.show.Loop
import org.slf4j.LoggerFactory

private[audio] class LoopInputImpl(loop: Loop) extends LoopInput {
  import LoopInputImpl._

  override def harder(): Unit = ???

  override def softer(): Unit = ???

  override def read(buffer: AudioBuffer): AudioEvent = ???

  override def close(): Unit =
    logger.debug("Loop input closed.")

  override def startDraining(): Unit = ???

  override def stopDraining(): Unit = ???

  override def toggleDraining(): Unit = ???
}

private object LoopInputImpl {
  private val logger = LoggerFactory.getLogger(classOf[LoopInputImpl])
}