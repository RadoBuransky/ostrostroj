package com.buransky.ostrostroj.app.audio

import java.nio.file.Path
import java.time.Duration

import com.buransky.ostrostroj.app.audio.impl.OstrostrojPlayer
import com.buransky.ostrostroj.app.audio.impl.input.PlaylistInputImpl
import com.buransky.ostrostroj.app.audio.impl.output.AsyncJavaxAudioOutput
import com.buransky.ostrostroj.app.audio.impl.provider.AsyncAudioProvider
import com.buransky.ostrostroj.app.common.OstrostrojException
import com.buransky.ostrostroj.app.show.{Loop, Playlist, Song}
import com.sun.media.sound.WaveFileReader
import com.typesafe.config.Config
import javax.sound.sampled.spi.AudioFileReader
import javax.sound.sampled._
import org.slf4j.LoggerFactory

case class AudioPlayerStatus(song: Song,
                             position: FrameCount,
                             volume: Double,
                             isPlaying: Boolean,
                             done: Boolean,
                             looping: Option[AudioPlayerLoopingStatus])
case class AudioPlayerLoopingStatus(loop: Loop,
                                    level: Int,
                                    minLevel: Int,
                                    maxLevel: Int,
                                    isDraining: Boolean)

/**
 * Main interface for playback control.
 */
trait AudioPlayer extends AutoCloseable {
  def play(): Unit
  def pause(): Unit

  def toggleLooping(): Unit

  def harder(): Option[Int]
  def softer(): Option[Int]

  def volumeUp(): Double
  def volumeDown(): Double

  def status: AudioPlayerStatus
}

object AudioPlayer {
  private val logger = LoggerFactory.getLogger(classOf[AudioPlayer])

  def apply(playlist: Playlist, audioConfig: Config): AudioPlayer = {
    playlist.checkFilesExist()
    val mixer = getMixer(audioConfig.getString("mixerName"))
    val bufferLength = Duration.ofMillis(audioConfig.getInt("bufferLengthMs"))
    val audioFileReader = new WaveFileReader()
    val sourceDataLine = getSourceDataLine(playlist.songs.head.path, mixer.getMixerInfo, bufferLength, audioFileReader)
    val audioOutput = new AsyncJavaxAudioOutput(sourceDataLine, audioConfig.getInt("bufferCount"))
    val audioMixer = AudioMixer(sourceDataLine.getFormat)
    val playlistInput = new PlaylistInputImpl(playlist, SongInput.apply(_, _, audioMixer), audioFileReader)
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
    if (logger.isTraceEnabled) {
      logSourceDataLineInfo(sourceDataLine.getLineInfo)
    }
    val bufferSize = bufferLength.toMillis*audioFormat.getSampleRate*audioFormat.getFrameSize/1000
    sourceDataLine.open(audioFormat, bufferSize.toInt)
    logger.info(s"Source data line open. [${audioFormat.getSampleRate}, ${audioFormat.getSampleSizeInBits}, " +
      s"${audioFormat.getChannels}, ${audioFormat.isBigEndian}, buffer size = ${sourceDataLine.getBufferSize}]")
    sourceDataLine
  }

  private def logSourceDataLineInfo(i: Line.Info): Unit = {
    i match {
      case dli: DataLine.Info =>
        val formats = dli.getFormats.map(f => s"[${f.getSampleRate},${f.getSampleSizeInBits},${f.getChannels}," +
          s"${f.isBigEndian},${f.getEncoding}]").mkString(",")
        logger.trace(s"[${dli.getMinBufferSize},${dli.getMaxBufferSize}] $formats")
      case _ => logger.trace(i.getLineClass.toString)
    }
  }

  private def getMixer(mixerName: String): Mixer = {
    if (logger.isTraceEnabled) {
      AudioSystem.getMixerInfo.foreach { mixerInfo =>
        logger.trace(s"${mixerInfo.getName}; ${mixerInfo.getDescription}; ${mixerInfo.getVendor}")
      }
    }

    val mixerInfo = AudioSystem.getMixerInfo.find { mixerInfo: Mixer.Info =>
      val mixer: Mixer = AudioSystem.getMixer(mixerInfo)
      val sourceLineInfos = mixer.getSourceLineInfo
      (sourceLineInfos != null) && sourceLineInfos.nonEmpty && mixerInfo.getName.contains(mixerName)
    }
      .getOrElse(throw new OstrostrojException(s"Audio mixer for not found! [$mixerName]"))
    val mixer = AudioSystem.getMixer(mixerInfo)
    mixer.open()
    logger.info(s"Audio mixer is open. [${mixer.getMixerInfo.getName}]")
    mixer
  }
}