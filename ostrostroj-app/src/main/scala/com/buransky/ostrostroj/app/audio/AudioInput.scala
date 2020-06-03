package com.buransky.ostrostroj.app.audio

import com.buransky.ostrostroj.app.audio.impl.input.{LoadedTrack, LoopInputImpl, SongInputImpl}
import com.buransky.ostrostroj.app.common.OstrostrojException
import com.buransky.ostrostroj.app.show.{Loop, Song}
import javax.sound.sampled.spi.AudioFileReader

private[audio] trait AudioInput extends AutoCloseable {
  /**
   * Synchronously reads next audio data into provided byte buffer.
   * @param buffer Buffer to read audio data into.
   * @return Fill
   */
  def read(buffer: AudioBuffer): AudioBuffer
}

private[audio] case class LoopStatus(loop: Loop,
                                     level: Int,
                                     minLevel: Int,
                                     maxLevel: Int,
                                     isDraining: Boolean,
                                     position: FrameCount)

/**
 * Reads audio data of a loop within a song. Handles changing of velocity levels.
 */
private[audio] trait LoopInput extends AudioInput {
  def harder(): Unit
  def softer(): Unit

  def startDraining(): Unit
  def stopDraining(): Unit
  def toggleDraining(): Unit
  def status: LoopStatus
}

private[audio] case class SongStatus(song: Song,
                                     loopStatus: Option[LoopStatus],
                                     position: FrameCount)

/**
 * Reads audio data of a song within a playlist. Handles starting and stopping of looping.
 */
private[audio] trait SongInput extends AudioInput {
  def startLooping(): Unit
  def stopLooping(): Unit
  def toggleLooping(): Unit
  def loopInput: Option[LoopInput]
  def status: SongStatus
}

private[audio] case class PlaylistStatus(songStatus: SongStatus)

/**
 * Reads audio data of a playlist. Handles skipping to the next song after the current song is done.
 */
private[audio] trait PlaylistInput extends AudioInput {
  def songInput: SongInput
  def status: PlaylistStatus
}

private[audio] object LoopInput {
  def apply(loop: Loop, startingPosition: FrameCount, audioFileReader: AudioFileReader,
            audioMixer: AudioMixer): LoopInput = {
    val trackAudioBuffers = loop.tracks.map(t => AudioBuffer(t.path, audioFileReader))
    val loadedTracks = loop.tracks.zip(trackAudioBuffers).map(i => LoadedTrack(i._1, i._2))
    new LoopInputImpl(loop, loadedTracks, startingPosition, audioMixer)
  }
}

private[audio] object SongInput {
  def apply(song: Song, audioFileReader: AudioFileReader, audioMixer: AudioMixer): SongInput = {
    val masterTrackInputStream = try {
      audioFileReader.getAudioInputStream(song.path.toFile)
    } catch {
      case ex: Throwable => throw new OstrostrojException(s"Can't get audio input stream! [${song.path}]", ex)
    }
    new SongInputImpl(song, masterTrackInputStream, LoopInput.apply(_, _, audioFileReader, audioMixer))
  }
}