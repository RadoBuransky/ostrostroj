package org.jaudiolibs.jnajack

import com.buransky.ostrostroj.player.OstrostrojException

import java.util
import scala.util.control.NonFatal

class OstrostrojJackClient(val underlying: JackClient) extends AutoCloseable {
  def connect(sourcePort: String, destinationPort: String): Unit = {
    val result = try {
      underlying.jackLib.jack_connect(underlying.clientPtr, sourcePort, destinationPort)
    } catch {
      case NonFatal(ex) =>
        throw new OstrostrojException(s"Connect failed! [$sourcePort, $destinationPort]", ex)
    }
    if (result != 0) {
      throw new OstrostrojException(s"Connect failed! [$sourcePort, $destinationPort, $result]")
    }
  }

  override def close(): Unit = {
    underlying.close()
  }
}

object OstrostrojJackClient {
  def apply(jack: Jack): OstrostrojJackClient = {
    val status = util.EnumSet.noneOf(classOf[JackStatus])
    try {
      val jackClient = jack.openClient("ostrostroj", util.EnumSet.of(JackOptions.JackNoStartServer), status)
      new OstrostrojJackClient(jackClient)
    } catch {
      case NonFatal(ex) =>
        throw new OstrostrojException(s"Open failed! $status", ex)
    }
  }
}