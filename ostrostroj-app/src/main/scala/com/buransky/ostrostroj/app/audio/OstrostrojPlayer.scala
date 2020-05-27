package com.buransky.ostrostroj.app.audio

import com.buransky.ostrostroj.app.audio.impl.OstrostrojPlayerImpl
import com.buransky.ostrostroj.app.show.Playlist

/**
 * Main interface for playback control.
 */
trait OstrostrojPlayer extends AutoCloseable {
  def selectSong(songIndex: Int): Unit

  def start(): Unit
  def stop(): Unit

  def startLooping(): Unit
  def stopLooping(): Unit

  def harder(): Unit
  def softer(): Unit

  def setVolume(volume: Int): Unit

  def getStatus(): OstrostrojPlayerStatus
}

object OstrostrojPlayer {
  def apply(playlist: Playlist): OstrostrojPlayer = new OstrostrojPlayerImpl(playlist)
}

case class OstrostrojPlayerStatus()