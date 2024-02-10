package com.buransky.ostrostroj.player

import com.buransky.ostrostroj.player.midi.MidiCommands
import org.jaudiolibs.jnajack.{Jack, JackPortFlags, JackPortType, OstrostrojJackClient}
import org.slf4j.LoggerFactory

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
    val jack = Jack.getInstance()
    val jackClient = OstrostrojJackClient(jack)
    try {
      jackClient.underlying.activate()
      log.info(s"Jack audio client [${jackClient.underlying.getName}] activated.")
      val port = jackClient.underlying.registerPort("p1", JackPortType.AUDIO, JackPortFlags.JackPortIsOutput)
      log.info(s"Port created. [${port.getName}, ${port.getType}, ${port.getConnections.length}]")
      jackClient.connect(port.getName, "alsa_pcm:hw:UMC1820:in1")
      log.info("Ports connected.")
    } finally {
      jackClient.close()
    }

    new Player(Vector.empty)
  }

  private def debugJavaSound(): Unit = {
    logDebugInfo()
    val format = new AudioFormat(44100f, 16, 1, true, false);
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
                case NonFatal(ex) =>
                  log.warn(s"$format failed!", ex)
                  try {
                    sourceDataLine.open()
                    try {
                      log.info(s"  Source line open: ${sourceDataLine.getFormat}")
                    } finally {
                      sourceDataLine.close()
                    }
                  } catch {
                    case NonFatal(ex) =>
                      log.warn(s"No format failed too!", ex)
                  }
              }
            }
          case _ =>
        }
      }
    }
  }

  private def logDebugInfo(): Unit = {
    log.info("Debug info start.")
    AudioSystem.getMixerInfo.foreach { mixerInfo =>
      val mixer = AudioSystem.getMixer(mixerInfo)
      log.info(s"Mixer: ${mixerInfo.getName} (${mixerInfo.getDescription})")
      mixer.getSourceLineInfo().foreach {
        case dataLineInfo: DataLine.Info =>
          log.info(s"  Data line info: ${dataLineInfo}")
          dataLineInfo.getFormats.foreach { format =>
            log.info(s"    $format")
          }
        case portInfo: Port.Info =>
          log.info(s"  Port info: ${portInfo}")
        case lineInfo =>
          log.info(s"  Source line info: ${lineInfo}")
      }
      mixer.getTargetLineInfo().foreach { lineInfo =>
        log.info(s"  Target line info: ${lineInfo}")
      }
    }
    log.info("Debug info end.")
  }
}