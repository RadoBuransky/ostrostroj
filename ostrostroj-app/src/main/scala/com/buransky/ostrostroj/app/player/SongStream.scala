package com.buransky.ostrostroj.app.player

import java.nio.ByteBuffer
import java.nio.file.Path

import javax.sound.sampled.AudioFormat

import scala.concurrent.Future

class SongStream(tracks: Seq[Path],
                 audioFormat: AudioFormat) extends AutoCloseable {
  private val loopingInputStreams = tracks.map(new LoopingInputStream(_, audioFormat))
  private val mutingInputStreams = loopingInputStreams.map(new MutingInputStream(_))
  private val mixingInputStream = new MixingInputStream(mutingInputStreams, audioFormat)
  private val bufferedInputStream = new BufferedInputStream(mixingInputStream)

  def byteBuffer: Future[ByteBuffer] = bufferedInputStream.byteBuffer

  override def close(): Unit = {
    bufferedInputStream.close()
    mixingInputStream.close()
    mutingInputStreams.foreach(_.close())
    loopingInputStreams.foreach(_.close())
  }
}