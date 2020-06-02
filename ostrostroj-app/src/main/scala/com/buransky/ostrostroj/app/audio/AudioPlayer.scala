package com.buransky.ostrostroj.app.audio

import com.buransky.ostrostroj.app.audio.impl.OstrostrojPlayer
import com.buransky.ostrostroj.app.audio.impl.provider.AsyncAudioProvider
import com.buransky.ostrostroj.app.show.Playlist
import javax.sound.sampled.SourceDataLine

case class AudioPlayerStatus()

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

  def status: AudioPlayerStatus
}

object AudioPlayer {
  def apply(playlist: Playlist): AudioPlayer = {
    val sourceDataLine: SourceDataLine = ???
    val audioOutput: AudioOutput = ???
    val playlistInput: PlaylistInput = ???
    val asyncAudioProvider: AsyncAudioProvider = ???
    new OstrostrojPlayer(sourceDataLine, audioOutput, playlistInput, asyncAudioProvider)
  }
}