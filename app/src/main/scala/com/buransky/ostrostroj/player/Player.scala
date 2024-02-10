package com.buransky.ostrostroj.player

import com.buransky.ostrostroj.player.midi.MidiCommands
import org.jaudiolibs.jnajack._
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
    val sineSize = 200
    val sine = (0 until sineSize).map { i =>
      (0.2 * Math.sin((i.toDouble / sineSize.toDouble) * Math.PI * 2.0)).toFloat
    }.toVector
    @volatile var sinePosition = 0

    val jack = Jack.getInstance()
    val jackClient = OstrostrojJackClient(jack)
    try {
//      jack.getPorts(jackClient.underlying, null, JackPortType.AUDIO,
//        util.EnumSet.of(JackPortFlags.JackPortIsInput, JackPortFlags.JackPortIsPhysical)).foreach { port =>
//        log.info(s"Input port: $port")
//      }

      log.info(s"Jack audio client [${jackClient.underlying.getName}, ${jackClient.underlying.getSampleRate}Hz, " +
        s"${jackClient.underlying.getBufferSize} bytes] activated.")
      val port = jackClient.underlying.registerPort("p1", JackPortType.AUDIO, JackPortFlags.JackPortIsOutput)
      log.info(s"Port created. [${port.getName}, ${port.getType.getBufferSize}]")

      jackClient.underlying.setProcessCallback((_: JackClient, nframes: Int) => {
        val floatBuffer = port.getFloatBuffer
        val limit = Math.min(nframes, floatBuffer.capacity())
        for (i <- 0 until limit) {
          floatBuffer.put(i, sine(sinePosition))
          sinePosition = (sinePosition + 1) % sineSize
        }
        true
      })
      jackClient.underlying.activate()

      jack.connect(jackClient.underlying, port.getName, "alsa_pcm:hw:UMC1820:in1")
      log.info(s"Ports connected. ${port.getConnections.mkString("[", ",", "]")}")

      Thread.sleep(5000)
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