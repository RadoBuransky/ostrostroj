package com.buransky.ostrostroj.app.audio

private[audio] trait AudioInput extends AutoCloseable {
  /**
   * Synchronously reads next audio data into provided byte buffer.
   * @param buffer Buffer to read audio data into.
   * @return Fill
   */
  def read(buffer: AudioBuffer): AudioBuffer
}

/**
 * Reads audio data of a loop within a song. Handles changing of velocity levels.
 */
private[audio] trait LoopInput extends AudioInput {
  def harder(): Unit
  def softer(): Unit

  def startDraining(): Unit
  def stopDraining(): Unit
  def toggleDraining(): Unit
}

/**
 * Reads audio data of a song within a playlist. Handles starting and stopping of looping.
 */
private[audio] trait SongInput extends AudioInput {
  def startLooping(): Unit
  def stopLooping(): Unit
  def toggleLooping(): Unit
  def loopInput: Option[LoopInput]
}

/**
 * Reads audio data of a playlist. Handles skipping to the next song after the current song is done.
 */
private[audio] trait PlaylistInput extends AudioInput {
  def songInput: SongInput
}