package com.buransky.ostrostroj.app.player.looper

import java.nio.ByteBuffer

import com.buransky.ostrostroj.app.common.OstrostrojException
import com.buransky.ostrostroj.app.player.PlaylistPlayer.LoopStatus
import com.buransky.ostrostroj.app.player.{BytePosition, SamplePosition}
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
  /**
   * Position - where to start reading
   * Limit - where to end reading & start writing
   * Capacity - where to end writing
   */
  private val buffer: ByteBuffer = ByteBuffer.allocate(bufferSize)
  buffer.limit(0)
  private var masterStreamPosition: BytePosition = BytePosition(masterFileFormat.getFormat, 0)
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
  def streamPosition: BytePosition = synchronized {
    loopLooper match {
      case Some(l) => l.streamPosition
      case None => masterStreamPosition
    }
  }

  def loopStatus: Option[LoopStatus] = synchronized {
    loopLooper.map {l =>
      LoopStatus(
        start = SamplePosition(masterFileFormat.getFormat, l.loop.start),
        end = SamplePosition(masterFileFormat.getFormat, l.loop.endExclusive),
        minLevel = l.loop.levels.map(_.level).min,
        maxLevel = l.loop.levels.map(_.level).max,
        currentLevel = l.currentLevel,
        targetLevel = l.targetLevel,
        isDraining = l.draining,
        counter = l.counter
      )
    }
  }

  private def fillFromMaster(): Unit = {
    if (buffer.limit() < buffer.capacity()) {
      val bytesRead = masterStream.read(buffer.array(), buffer.limit(), buffer.capacity() - buffer.limit())
      if (bytesRead > 0) {
        buffer.limit(buffer.limit() + bytesRead)
        masterStreamPosition = masterStreamPosition.add(bytesRead)
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
        if (masterSkip.bytePosition > 0) {
          masterStreamPosition = masterStreamPosition.add(masterStream.skip(masterSkip.bytePosition).toInt)
        }
      }
    }
  }

  private def loopAtPosition(position: BytePosition): Option[Loop] = {
    val samplePosition = position.toSample.samplePosition
    song.loops.find(l => l.start <= samplePosition && l.endExclusive > samplePosition)
  }

  private def checkAudioFormat(expected: AudioFormat, actual: AudioFormat): Boolean = {
    expected.getSampleRate == actual.getSampleRate && expected.getSampleSizeInBits == actual.getSampleSizeInBits &&
      expected.getChannels == actual.getChannels && expected.getEncoding == actual.getEncoding &&
      expected.isBigEndian == actual.isBigEndian
  }
}