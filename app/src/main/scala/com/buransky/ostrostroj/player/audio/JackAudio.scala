package com.buransky.ostrostroj.player.audio

import com.buransky.ostrostroj.player.OstrostrojException
import com.buransky.ostrostroj.player.audio.JackAudio.log
import org.jaudiolibs.jnajack.{Jack, JackClient, JackOptions, JackPort, JackPortFlags, JackPortType, JackProcessCallback, JackStatus}
import org.slf4j.LoggerFactory

import java.nio.FloatBuffer
import java.util
import scala.collection.mutable.ArrayBuffer
import scala.util.control.NonFatal

/**
 * Logs: ~/.log/jack/jackdbus.log
 */
class JackAudio(jack: Jack, jackClient: JackClient, ports: Vector[JackPort]) extends JackProcessCallback
  with AutoCloseable {
  /**
   * One sample per channel.
   */
  private val samples = ArrayBuffer.fill[FloatBuffer](ports.length)(null)

  def activate(channelPortNames: Vector[String]): Unit = {
    jackClient.setProcessCallback(this)
    jackClient.activate()
    log.info(s"Jack client activated. [${jackClient.getName}, ${jackClient.getSampleRate}Hz, " +
      s"${jackClient.getBufferSize} frames]")
    channelPortNames.indices.foreach { i =>
      jack.connect(jackClient, ports(i).getName, channelPortNames(i))
      log.info(s"  ${ports(i).getName} connected to ${channelPortNames(i)}")
    }
  }

  override def close(): Unit = {
    jackClient.close()
    log.info("Jack client closed.")
  }

  def play(channel: Int, sample: FloatBuffer): Unit = {
    samples.update(channel, sample)
  }

  override def process(client: JackClient, nframes: Int): Boolean = {
    ports.indices.foreach { channel =>
      try {
        val sample = samples(channel)
        if (sample != null) {
          if (sample.hasRemaining) {
            safeCopyFloatBuffer(sample, ports(channel).getFloatBuffer, nframes)
          } else {
            // Give the sample to GC
            samples.update(channel, null)
          }
        }
      } catch {
        case NonFatal(ex) =>
          log.error(s"Process failed! [$nframes]", ex)
      }
    }
    true
  }

  private def safeCopyFloatBuffer(src: FloatBuffer, dst: FloatBuffer, nframes: Int): Unit  ={
    val toCopy = math.min(math.min(src.remaining(), dst.remaining()), nframes)
    val oldLimit = src.limit()
    src.limit(src.position() + toCopy)
    dst.put(src)
    src.limit(oldLimit)
  }
}

object JackAudio {
  private val log = LoggerFactory.getLogger(classOf[JackAudio])
  private val jackClientName = "ostrostroj"
  private val jackClientOptions = util.EnumSet.of(JackOptions.JackNoStartServer)
  private val umc1820PlaybackPortNames = (1 to 10).map(i => s"alsa_pcm:hw:UMC1820:in$i").toVector

  def apply(): JackAudio = {
    apply(umc1820PlaybackPortNames)
  }

  def apply(channelPortNames: Vector[String]): JackAudio = {
    val callbackStatus = util.EnumSet.noneOf(classOf[JackStatus])
    try {
      val jack = Jack.getInstance()
      val jackClient = jack.openClient(jackClientName, jackClientOptions, callbackStatus)
      try {
        apply(channelPortNames, jack, jackClient)
      } catch {
        case NonFatal(ex) =>
          jackClient.close()
          throw ex
      }
    } catch {
      case NonFatal(ex) =>
        throw new OstrostrojException(s"Open failed! $callbackStatus", ex)
    }
  }

  private def apply(channelPortNames: Vector[String], jack: Jack, jackClient: JackClient): JackAudio = {
    val ports = registerPorts(channelPortNames.size, jackClient)
    val result = new JackAudio(jack, jackClient, ports)
    result.activate(channelPortNames)
    result
  }

  private def registerPorts(count: Int, jackClient: JackClient): Vector[JackPort] = {
    (1 to count).map { i =>
      jackClient.registerPort(i.toString, JackPortType.AUDIO, JackPortFlags.JackPortIsOutput)
    }.toVector
  }
}
