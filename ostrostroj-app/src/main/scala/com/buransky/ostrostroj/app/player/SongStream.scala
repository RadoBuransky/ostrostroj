package com.buransky.ostrostroj.app.player

import com.buransky.ostrostroj.app.audio.AudioMixer
import com.buransky.ostrostroj.app.common.OstrostrojException
import com.buransky.ostrostroj.app.show.Song
import javax.sound.sampled.AudioInputStream
import org.slf4j.LoggerFactory

import scala.annotation.tailrec
import scala.collection.mutable

class SongStream(song: Song, tracks: Seq[AudioInputStream], bufferSize: Int) extends AutoCloseable {
  private val loopers: Seq[Looper] = tracks.map(track => new Looper(song.loops, track))
  private val muted: mutable.Seq[Boolean] = mutable.Seq(song.tracks.map(_.muted):_*)
  private val mixingBuffer: Array[Byte] = new Array[Byte](bufferSize)

  def read(result: Array[Byte]): Int = synchronized {
    val bytesRead = read(0, result)
    readMuteAndMix(1, result, bytesRead)
    bytesRead
  }

  def mute(trackIndex: Int): Unit = synchronized { muted.update(trackIndex, true) }
  def unmute(trackIndex: Int): Unit = synchronized { muted.update(trackIndex, false) }
  def startLooping(): Unit = synchronized { loopers.foreach(_.startLooping()) }
  def stopLooping(): Unit = synchronized { loopers.foreach(_.stopLooping()) }

  override def close(): Unit = synchronized {
    tracks.foreach(_.close())
  }

  @tailrec
  private def readMuteAndMix(index: Int, result: Array[Byte], resultSize: Int): Unit = {
    if (index < loopers.length) {
      val bytesRead = read(index, mixingBuffer)
      if (bytesRead > 0) {
        if (resultSize != bytesRead) {
          throw new OstrostrojException(s"Different size read! [$resultSize, $bytesRead]")
        }
        if (!muted(index)) {
          AudioMixer.mix16bitLe(Seq(result, mixingBuffer), bytesRead)
        }
        readMuteAndMix(index + 1, result, bytesRead)
      }
    }
  }

  private def read(index: Int, result: Array[Byte]): Int = {
    val bytesRead = loopers(index).read(result)
    if (bytesRead == -1) {
      handleEndOfFile(index)
    } else {
      bytesRead
    }
  }

  private def handleEndOfFile(index: Int): Int = {
    if (index > 0) {
      throw new OstrostrojException(s"Unexpected end of file reached! [$index]")
    }
    if (!loopers.forall(_.read(mixingBuffer) == -1)) {
      throw new OstrostrojException(s"Not all files reached the end at the same time!")
    }
    -1
  }
}

object SongStream {
  private val logger = LoggerFactory.getLogger(classOf[SongStream])
}