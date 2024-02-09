package com.buransky.ostrostroj.player

import com.buransky.ostrostroj.player.midi.MidiCommands
import org.slf4j.LoggerFactory

import javax.sound.sampled.AudioFormat.Encoding
import javax.sound.sampled._
import scala.util.control.NonFatal

class Player(clips: Vector[Clip]) extends MidiCommands {
  private var index = 0

  override def start(): Unit = {
    clips(index).start()
  }

  override def stop(): Unit = {
    clips(index).stop()
  }

  override def continue(): Unit = {
    start()
  }

  override def programChange(num: Int): Unit = {
    if (num != index && num >= 0 && num < clips.size) {
      stop()
      index = num
      start()
    }
  }

  override def tick(): Unit = {
  }
}

object Player {
  private val log = LoggerFactory.getLogger(classOf[Player])

  def apply(): Player = {
    val format = new AudioFormat(44100f, 16, 2, true, false);
    val info = new DataLine.Info(classOf[SourceDataLine], format)
    AudioSystem.getMixerInfo.foreach { mixerInfo =>
      val mixer = AudioSystem.getMixer(mixerInfo)
      val sourceLineInfos = mixer.getSourceLineInfo(info)
      if (sourceLineInfos.nonEmpty) {
        log.info(s"Mixer: ${mixerInfo.getName} (${mixerInfo.getDescription})")
        sourceLineInfos.foreach {
          case dataLineInfo: DataLine.Info =>
            if (!dataLineInfo.isFormatSupported(format)) {
              dataLineInfo.getFormats.foreach { format =>
                log.info(s"  Format: $format")
              }
            } else {
              val sourceDataLine = mixer.getLine(dataLineInfo).asInstanceOf[SourceDataLine]
              try {
                sourceDataLine.open(format)
                try {
                  log.info(s"  Source line open: ${sourceDataLine.getFormat}")
                } finally {
                  sourceDataLine.close()
                }
              } catch {
                case NonFatal(ex) => log.warn(s"$format failed!", ex)
              }
            }
          case _ =>
        }
      }
    }
    new Player(Vector.empty)
  }
}