package com.buransky.ostrostroj.app.audio

import java.nio.file.Path
import java.util.concurrent.{ExecutorService, Executors}

import akka.actor.typed.scaladsl.{AbstractBehavior, ActorContext, Behaviors}
import akka.actor.typed.{ActorRef, Behavior, PostStop, Signal}
import com.buransky.ostrostroj.app.common.OstrostrojException
import javax.sound.sampled._
import org.slf4j.LoggerFactory

import scala.collection.mutable
import scala.concurrent.{ExecutionContext, Future}

/**
 * Multi-track audio player capable of looping, track un/muting and software down-mixing.
 *
 * Memory requirements math:
 * Max number of tracks = 5
 * Sampling rate / bits = 44.1 kHz / 16 bit
 * Bytes per second per track = 0.176 MB
 * Bytes per minute per track = 10.58 MB
 * Bytes per 4 min song, 5 tracks = 211.6 MB
 * Bytes per 10 min song, 5 tracks = 529 MB
 *
 */
object AudioPlayer {
  private val mixerName = "Digital Output" // Desktop mixer for testing
//  private val hifiShield = "ODROIDDAC" // Odroid HiFi Shield
  private val bufferSize = 4410 * 2 * 2; // 0.1 second @ 44.1 kHz, 16 bit, 2 channels
  private val logger = LoggerFactory.getLogger(AudioPlayer.getClass)

  final case class Position(sample: Int)

  sealed trait Event
  final case class PositionUpdated(current: Position) extends Event
  final case object PlaybackFinished

  sealed trait Command
  final case class Load(tracks: Seq[Path]) extends Command
  final case object Play extends Command
  final case object Pause extends Command
  final case class StartLooping(start: Position, end: Position) extends Command
  final case object StopLooping extends Command
  final case class UnmuteTrack(trackIndex: Int) extends Command
  final case class MuteTrack(trackIndex: Int) extends Command

  def apply(listener: ActorRef[_]): Behavior[Command] = Behaviors.setup { ctx =>
    if (logger.isTraceEnabled()) {
      logMixerInfo()
    }

    val mixer: Mixer = AudioSystem.getMixer(getMixerInfo)
    logger.info(s"Audio mixer open. [${mixer.getMixerInfo.getName}]")

    val executorService = Executors.newFixedThreadPool(1)

    Behaviors.receive[Command] {
      case (ctx, Load(tracks)) => new AudioPlayerBehavior(tracks, mixer, listener, executorService, ctx)
      case _ => Behaviors.ignore
    }.receiveSignal {
      case (_, PostStop) =>
        executorService.shutdown()
        Behaviors.same
    }
  }

  class AudioPlayerBehavior(tracks: Seq[Path],
                            mixer: Mixer,
                            listener: ActorRef[_],
                            executorService: ExecutorService,
                            ctx: ActorContext[Command]) extends AbstractBehavior[Command](ctx) with LineListener {
    private val mutedTracks = mutable.ArraySeq.fill[Boolean](tracks.length)(true)
    private val trackInputStreams: Seq[AudioInputStream] = load(tracks)
    private val sourceDataLine = AudioSystem.getSourceDataLine(trackInputStreams.head.getFormat, mixer.getMixerInfo)
    sourceDataLine.open(trackInputStreams.head.getFormat, bufferSize)
    logger.debug(s"Source data line open. [${trackInputStreams.head.getFormat}]")

    private val buffers = tracks.map(_ => new Array[Byte](sourceDataLine.getBufferSize)) // Bytes per sample * channels
    logger.debug(s"Buffer size = ${sourceDataLine.getBufferSize}")

    private val audioReaderWriterFuture: java.util.concurrent.Future[_] = executorService.submit(new AudioReaderWriter())

    sourceDataLine.addLineListener(this)

    override def onMessage(msg: Command): Behavior[Command] = msg match {
      case Load(newTracks) =>
        stop()
        new AudioPlayerBehavior(newTracks, mixer, listener, executorService, ctx)
      case Play =>
        if (!sourceDataLine.isRunning) {
          sourceDataLine.start()
          dataWriter(ctx.executionContext)
        }
        Behaviors.same
      case Pause =>
        sourceDataLine.stop()
        Behaviors.same
      case StartLooping(_, _) =>
        Behaviors.same
      case StopLooping =>
        Behaviors.same
      case UnmuteTrack(trackIndex) =>
        mutedTracks.update(trackIndex, false)
        logger.debug(s"Track unmuted. [$trackIndex]")
        Behaviors.same
      case MuteTrack(trackIndex) =>
        mutedTracks.update(trackIndex, true)
        logger.debug(s"Track muted. [$trackIndex]")
        Behaviors.same
    }

    override def onSignal: PartialFunction[Signal, Behavior[Command]] = {
      case PostStop =>
        stop()
        mixer.close()
        logger.info("Audio mixer closed.")
        Behaviors.same
    }

    override def update(event: LineEvent): Unit = {
      if (event != null) {
        logger.debug(s"Line event = ${event}")
      }
    }

    private def dataWriter(implicit executionContext: ExecutionContext): Future[_] = {
      Future {
        val bytesRead = fillBuffers()
        logger.debug(s"Bytes read = $bytesRead")

        if (bytesRead > -1) {
          mixBuffers(bytesRead)
          // TODO: What to do with bytesWritten?
          val bytesWritten = sourceDataLine.write(buffers.head, 0, bytesRead)
          logger.debug(s"Bytes written = $bytesWritten")

          // Read next data
          dataWriter(executionContext)
        }
      }
    }

    private def mixBuffers(bytesRead: Int): Unit = {
      val acc = buffers.head
      for (sample <- 0 until bytesRead) {
        var mix = 0
        for (track <- buffers.indices) {
          if (!mutedTracks(track)) {
            mix += buffers(track)(sample)
          }
        }
        acc.update(sample, mix.toByte)
      }
    }

    private def fillBuffers(): Int = {
      trackInputStreams.zip(buffers).map { case (stream, buffer) =>
        stream.read(buffer, 0, buffer.length)
      }.min
    }

    private def stop(): Unit = {
      sourceDataLine.close()
      logger.info("Source data line closed.")
      trackInputStreams.foreach { trackInputStream =>
        trackInputStream.close()
      }
      logger.info("Track input streams closed.")
    }

    private def load(tracks: Seq[Path]): Seq[AudioInputStream] = {
      val mainAudioFileFormat = AudioSystem.getAudioFileFormat(tracks.head.toFile)

      if (logger.isDebugEnabled) {
        logger.debug(s"Sampling frequency = ${mainAudioFileFormat.getFormat.getSampleRate}")
        logger.debug(s"Channels = ${mainAudioFileFormat.getFormat.getChannels}")
        logger.debug(s"Bits per sample = ${mainAudioFileFormat.getFormat.getSampleSizeInBits}")
        logger.debug(s"Encoding = ${mainAudioFileFormat.getFormat.getEncoding}")
      }

      tracks.map { trackPath =>
        val trackAudioFileFormat = AudioSystem.getAudioFileFormat(trackPath.toFile)
        // Make sure all audio files are using the same format
        if (trackAudioFileFormat.getFormat.getSampleRate.toInt != mainAudioFileFormat.getFormat.getSampleRate.toInt ||
          trackAudioFileFormat.getFormat.getChannels != mainAudioFileFormat.getFormat.getChannels ||
          trackAudioFileFormat.getFormat.getSampleSizeInBits != mainAudioFileFormat.getFormat.getSampleSizeInBits ||
          trackAudioFileFormat.getFormat.getEncoding != mainAudioFileFormat.getFormat.getEncoding)
          throw new OstrostrojException(s"Track [$trackPath] has different audio format! " +
            s"[${mainAudioFileFormat.getFormat} vs ${trackAudioFileFormat.getFormat}]")

        AudioSystem.getAudioInputStream(trackPath.toFile)
      }
    }
  }

  private def getMixerInfo: Mixer.Info = {
    AudioSystem.getMixerInfo.find(_.getName.contains(mixerName))
      .getOrElse(throw new OstrostrojException(s"Audio mixer for not found! [$mixerName]"))
  }

  private def logMixerInfo(): Unit = {
    try {
      AudioSystem.getMixerInfo.foreach { mixerInfo =>
        logger.trace(s"Name: ${mixerInfo.getName}")
        logger.trace(s"Vendor: ${mixerInfo.getVendor}")
        logger.trace(s"Description: ${mixerInfo.getDescription}")

        val mixer = AudioSystem.getMixer(mixerInfo)
        val sourceLinesInfo = mixer.getSourceLineInfo.map(_.toString)
          .mkString("Source lines:" + System.lineSeparator(), System.lineSeparator(), "")
        logger.trace(sourceLinesInfo)
      }
    }
    catch {
      case _: Throwable => // Ignore intentionally
    }
  }
}
