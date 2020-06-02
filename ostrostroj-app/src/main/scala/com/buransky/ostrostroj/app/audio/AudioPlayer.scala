package com.buransky.ostrostroj.app.audio

import com.buransky.ostrostroj.app.audio.impl.OstrostrojPlayer
import com.buransky.ostrostroj.app.show.Playlist

/**
 * Main interface for playback control.
 */
trait AudioPlayer extends AutoCloseable {
  def play(): Unit
  def pause(): Unit

  def startLooping(): Unit
  def stopLooping(): Unit

  def harder(): Unit
  def softer(): Unit

  def setVolume(volume: Int): Unit

  def getStatus(): OstrostrojPlayerStatus
}

object AudioPlayer {
  def apply(playlist: Playlist): AudioPlayer = ???
}

case class OstrostrojPlayerStatus()