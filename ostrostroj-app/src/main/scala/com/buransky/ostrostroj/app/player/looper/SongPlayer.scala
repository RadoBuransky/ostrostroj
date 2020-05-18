package com.buransky.ostrostroj.app.player.looper

import java.nio.ByteBuffer

import com.buransky.ostrostroj.app.common.OstrostrojException
import com.buransky.ostrostroj.app.player.PlaylistPlayer
import com.buransky.ostrostroj.app.player.PlaylistPlayer.LoopStatus
import com.buransky.ostrostroj.app.show.{Loop, Song}
import javax.sound.sampled.{AudioFileFormat, AudioFormat, AudioInputStream, AudioSystem}

class SongPlayer(song: Song, bufferSize: Int) extends AutoCloseable {
  private val masterFileFormat: AudioFileFormat = {
    AudioSystem.getAudioFileFormat(song.path.toFile)
  }
  private val masterStream: AudioInputStream = {
    val audioInputStream = AudioSystem.getAudioInputStream(song.path.toFile)
    if (!checkAudioFormat(masterFileFormat.getFormat, audioInputStream.getFormat)) {
      throw new OstrostrojException(s"Not the same audio format! [${song.path}]")
    }
    audioInputStream
  }
  private val buffer: ByteBuffer = ByteBuffer.allocate(bufferSize)
  buffer.limit(0)
  private var masterStreamPosition: Int = 0
  private var loopLooper: Option[LoopLooper] = None

  override def close(): Unit = synchronized {
    masterStream.close()
  }

  def fillBuffer(): ByteBuffer = synchronized {
    if (buffer.position() == buffer.limit()) {
      buffer.position(0)
      buffer.limit(0)
      fillFromLooper()
      fillFromMaster()
    }
    buffer
  }

  def startLooping(): Unit = synchronized {
    loopLooper match {
      case Some(l) => l.stopDraining()
      case None => loopLooper = loopAtPosition(masterStreamPosition).map(LoopLooper(_, masterFileFormat.getFormat))
    }
  }

  def stopLooping(): Unit = synchronized {
    loopLooper.foreach(_.startDraining())
  }

  def harder(): Unit = synchronized {
    loopLooper.foreach(_.harder())
  }

  def softer(): Unit = synchronized {
    loopLooper.foreach(_.softer())
  }

  def fileFormat: AudioFileFormat = synchronized { masterFileFormat }
  def streamPosition: Int = synchronized {
    loopLooper match {
      case Some(l) => l.streamPosition
      case None => masterStreamPosition
    }
  }

  def loopStatus: Option[LoopStatus] = synchronized {
    loopLooper.map {l =>
      LoopStatus(
        start = PlaylistPlayer.framePositionToDuration(l.loop.start, fileFormat.getFormat.getSampleRate),
        end = PlaylistPlayer.framePositionToDuration(l.loop.endExclusive, fileFormat.getFormat.getSampleRate),
        minLevel = l.loop.tracks.map(_.level).min,
        maxLevel = l.loop.tracks.map(_.level).max,
        currentLevel = l.currentLevel,
        targetLevel = l.targetLevel,
        isDraining = l.draining
      )
    }
  }

  private def fillFromMaster(): Unit = {
    if (buffer.limit() < buffer.capacity()) {
      val bytesRead = masterStream.read(buffer.array(), buffer.limit(), buffer.capacity() - buffer.limit())
      if (bytesRead > 0) {
        buffer.limit(buffer.limit() + bytesRead)
        masterStreamPosition += bytesRead
      }
    }
  }

  private def fillFromLooper(): Unit = {
    loopLooper.foreach { l =>
      val oldLimit = buffer.limit()
      val masterSkip = l.fill(buffer, masterStreamPosition)
      if (buffer.limit() - oldLimit == 0) {
        buffer.limit(0)
        loopLooper = None
      } else {
        if (masterSkip > 0) {
          masterStreamPosition += masterStream.skip(masterSkip).toInt
        }
      }
    }
  }

  private def loopAtPosition(position: Int): Option[Loop] =
    song.loops.find(l => l.start <= position && l.endExclusive > position)

  private def checkAudioFormat(expected: AudioFormat, actual: AudioFormat): Boolean = {
    expected.getSampleRate == actual.getSampleRate && expected.getSampleSizeInBits == actual.getSampleSizeInBits &&
      expected.getChannels == actual.getChannels && expected.getEncoding == actual.getEncoding &&
      expected.isBigEndian == actual.isBigEndian
  }
}