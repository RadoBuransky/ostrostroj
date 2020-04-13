package com.buransky.ostrostroj.app.audio

import akka.NotUsed
import akka.actor.typed.Behavior

/**
 * Multi-track audio player capable of looping, track un/muting and software down-mixing.
 */
object AudioPlayer {
  def apply(): Behavior[NotUsed] = ???
}
