package com.buransky.ostrostroj.app.player

import com.buransky.ostrostroj.app.show.Song
import javax.sound.sampled.AudioInputStream

import scala.annotation.tailrec
import scala.collection.mutable

class SongStream(song: Song, tracks: Seq[AudioInputStream]) extends AutoCloseable {
  private val loopers: Seq[Looper] = tracks.map(track => new Looper(song.loops, track))
  private val muted: mutable.Seq[Boolean] = ???
  private val mixingBuffer: Array[Byte] = ???

  def read(result: Array[Byte]): Int = readMuteAndMix(0, result, 0)

  override def close(): Unit = {
    tracks.foreach(_.close())
  }

  @tailrec
  private def readMuteAndMix(index: Int, result: Array[Byte], resultSize: Int): Int = {
    if (index == loopers.length) {
      resultSize
    } else {
      val bytesRead = loopers(index).read(mixingBuffer)
      if (bytesRead < 1) {
        // End of file
        0
      } else {
        if (!muted(index)) {
          // TODO: Mix "buffer" into the "result"
        }
        readMuteAndMix(index + 1, result, bytesRead)
      }
    }
  }
}