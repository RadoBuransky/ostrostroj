package com.buransky.ostrostroj.app.audio

import com.buransky.ostrostroj.app.audio.impl.provider.AsyncAudioProvider

/**
 * Implementation is responsible for connecting audio input and output.
 */
private[audio] trait AudioProvider extends AutoCloseable {
  /**
   * Waits for free buffer available by output, fills it and queues it.
   * @return false if end of stream has been reached, true otherwise
   */
  def waitReadAndQueue(audioInput: AudioInput, audioOutput: AudioOutput): Boolean
}

private[audio] object AudioProvider {
  def apply(audioInput: AudioInput, audioOutput: AudioOutput): AudioProvider = {
    new AsyncAudioProvider(audioInput, audioOutput)
  }
}