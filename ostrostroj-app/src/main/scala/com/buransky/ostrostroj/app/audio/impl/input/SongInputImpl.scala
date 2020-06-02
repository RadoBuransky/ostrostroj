package com.buransky.ostrostroj.app.audio.impl.input

import com.buransky.ostrostroj.app.audio.{AudioBuffer, AudioEvent, LoopInput, SongInput}

private[audio] class SongInputImpl extends SongInput {
  override def startLooping(): Unit = ???

  override def stopLooping(): Unit = ???

  override def loopInput: Option[LoopInput] = ???

  /**
   * Synchronously reads next audio data into provided byte buffer.
   *
   * @param buffer Buffer to read audio data into.
   * @return Fill
   */
  override def read(buffer: AudioBuffer): AudioEvent = ???

  override def close(): Unit = ???
}
