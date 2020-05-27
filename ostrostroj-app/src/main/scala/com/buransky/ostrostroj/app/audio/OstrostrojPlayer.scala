package com.buransky.ostrostroj.app.audio

import com.buransky.ostrostroj.app.audio.impl.JavaSoundOstrostrojPlayer
import com.buransky.ostrostroj.app.show.Playlist

/**
 * Main interface for playback control.
 */
trait OstrostrojPlayer extends AutoCloseable {
  def start(songIndex: Int): Unit
  def pause(): Unit
  def resume(): Unit

  def startLooping(): Unit
  def stopLooping(): Unit

  def harder(): Unit
  def softer(): Unit

  def setVolume(volume: Int): Unit

  def getStatus(): OstrostrojPlayerStatus
}

object OstrostrojPlayer {
  def apply(playlist: Playlist): OstrostrojPlayer = new JavaSoundOstrostrojPlayer(playlist)
}

case class OstrostrojPlayerStatus()