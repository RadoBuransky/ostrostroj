package com.buransky.ostrostroj.app.audio

import java.nio.file.Path
import java.time.Duration

import com.buransky.ostrostroj.app.audio.impl.OstrostrojPlayer
import com.buransky.ostrostroj.app.audio.impl.input.PlaylistInputImpl
import com.buransky.ostrostroj.app.audio.impl.output.AsyncJavaxAudioOutput
import com.buransky.ostrostroj.app.audio.impl.provider.AsyncAudioProvider
import com.buransky.ostrostroj.app.common.OstrostrojException
import com.buransky.ostrostroj.app.show.Playlist
import com.sun.media.sound.WaveFileReader
import com.typesafe.config.Config
import javax.sound.sampled.spi.AudioFileReader
import javax.sound.sampled.{AudioSystem, Mixer, SourceDataLine}
import org.slf4j.LoggerFactory

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
  private val logger = LoggerFactory.getLogger(classOf[AudioPlayer])

  def apply(playlist: Playlist, audioConfig: Config): AudioPlayer = {
    val mixer = getMixer(audioConfig.getString("mixerName"))
    val bufferLength = Duration.ofMillis(audioConfig.getInt("bufferLength"))
    val audioFileReader = new WaveFileReader()
    val sourceDataLine = getSourceDataLine(playlist.songs.head.path, mixer.getMixerInfo, bufferLength, audioFileReader)
    val audioOutput = new AsyncJavaxAudioOutput(sourceDataLine, audioConfig.getInt("bufferCount"))
    val playlistInput = new PlaylistInputImpl(playlist, SongInput.apply, audioFileReader)
    val audioProvider = new AsyncAudioProvider(playlistInput, audioOutput)
    new OstrostrojPlayer(sourceDataLine, audioOutput, playlistInput, audioProvider)
  }

  private def getSourceDataLine(firstSongMaster: Path,
                                mixerInfo: Mixer.Info,
                                bufferLength: Duration,
                                audioFileReader: AudioFileReader): SourceDataLine = {
    val audioFileFormat = audioFileReader.getAudioFileFormat(firstSongMaster.toFile)
    val audioFormat = audioFileFormat.getFormat
    val sourceDataLine = AudioSystem.getSourceDataLine(audioFormat, mixerInfo)
    val bufferSize = bufferLength.toMillis*audioFormat.getSampleRate*(audioFormat.getSampleSizeInBits/8)*
      audioFormat.getChannels/1000
    sourceDataLine.open(audioFormat, bufferSize.toInt)
    logger.info(s"Source data line open. [${audioFormat.getSampleRate}, ${audioFormat.getSampleSizeInBits}, " +
      s"${audioFormat.getChannels}, buffer size = ${sourceDataLine.getBufferSize}]")
    sourceDataLine
  }

  private def getMixer(mixerName: String): Mixer = {
    val mixerInfo = AudioSystem.getMixerInfo.find(_.getName.contains(mixerName))
      .getOrElse(throw new OstrostrojException(s"Audio mixer for not found! [$mixerName]"))
    val mixer = AudioSystem.getMixer(mixerInfo)
    mixer.open()
    logger.info(s"Audio mixer is open. [${mixer.getMixerInfo.getName}]")
    mixer
  }
}