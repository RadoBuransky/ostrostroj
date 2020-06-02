package com.buransky.ostrostroj.app.audio.impl.input

import com.buransky.ostrostroj.app.audio.{AudioBuffer, AudioEvent, LoopInput}

private[audio] class LoopInputImpl extends LoopInput {
  override def harder(): Unit = ???

  override def softer(): Unit = ???

  /**
   * Synchronously reads next audio data into provided byte buffer.
   *
   * @param buffer Buffer to read audio data into.
   * @return Fill
   */
  override def read(buffer: AudioBuffer): AudioEvent = ???

  override def close(): Unit = ???
}
