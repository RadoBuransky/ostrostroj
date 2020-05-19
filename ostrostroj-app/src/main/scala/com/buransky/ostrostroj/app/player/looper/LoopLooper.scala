package com.buransky.ostrostroj.app.player.looper

import java.nio.ByteBuffer

import com.buransky.ostrostroj.app.common.OstrostrojException
import com.buransky.ostrostroj.app.player.{BytePosition, SamplePosition}
import com.buransky.ostrostroj.app.show.Loop
import javax.sound.sampled.{AudioFormat, AudioSystem}

import scala.annotation.tailrec

class LoopLooper(val loop: Loop, audioFormat: AudioFormat, levelBuffers: Map[Int, Array[Byte]]) {
  private val loopStart = SamplePosition(audioFormat, loop.start)

  /**
   * Small buffer used only for crossfading between levels.
   */
  private val xfadeBuffer: ByteBuffer = ByteBuffer.allocate(BufferMixer.xfadingBufferLength(audioFormat))

  /**
   * Current loop level index.
   */
  private var _currentLevel: Int = 0

  /**
   * Lower and higher limits for the current level.
   */
  private var currentLevelLimits: (Int, Int) = (0, 0)

  /**
   * Target level to get to
   */
  private var _targetLevel: Int = 0
  private var looperPosition: Option[BytePosition] = None
  private var _draining: Boolean = false

  /**
   * @return Number of bytes to skip in the master stream.
   */
  def fill(buffer: ByteBuffer, masterStreamPosition: BytePosition): BytePosition = synchronized {
    if (looperPosition.isEmpty) {
      initLooperPosition(masterStreamPosition)
    }

    looperPosition match {
      case Some(lp) =>
        val bufferPosition = BytePosition(audioFormat, lp.bytePosition - loopStart.toByte.bytePosition)
        if (bufferPosition.bytePosition == 0 && _draining) {
          BytePosition(audioFormat, 0)
        } else {
          val dstBuffer = ByteBuffer.wrap(buffer.array())
          dstBuffer.position(buffer.limit())
          dstBuffer.limit(buffer.capacity())

          val newBufferPosition = read(dstBuffer, bufferPosition)

          looperPosition = Some(BytePosition(audioFormat, loop.start + newBufferPosition.bytePosition))
          buffer.limit(dstBuffer.position())
          createResult(masterStreamPosition)
        }
      case None => throw new OstrostrojException("Looper position unknown!")
    }
  }

  def harder(): Unit = setTargetLevel(_targetLevel + 1)
  def softer(): Unit = setTargetLevel(_targetLevel - 1)
  def startDraining(): Unit = synchronized { _draining = true }
  def stopDraining(): Unit = synchronized { _draining = false }
  def streamPosition: BytePosition = synchronized {
    looperPosition.getOrElse(BytePosition(audioFormat, 0))
  }
  def currentLevel: Int = _currentLevel
  def targetLevel: Int = _targetLevel
  def draining: Boolean = _draining

  private def createResult(masterStreamPosition: BytePosition): BytePosition = {
    val loopEndPosition = loop.endExclusive * audioFormat.getChannels * audioFormat.getSampleSizeInBits / 8
    BytePosition(audioFormat, loopEndPosition - masterStreamPosition.bytePosition)
  }

  @tailrec
  private def read(dst: ByteBuffer, bufferPosition: BytePosition): BytePosition = {
    if (bufferPosition.bytePosition == 0 && _draining) {
      // Draining done, we're done with this loop
      bufferPosition
    } else {
      val newBufferPositionValue = if (_currentLevel == _targetLevel || bufferPosition.bytePosition != 0) {
        currentLevelToBuffer(dst)
      } else {
        // Crossfade only at the beginning of the loop
        xfadeNextLevelToBuffer(dst)
      }

      // Read until buffer is full or draining
      val newBufferPosition = BytePosition(audioFormat, newBufferPositionValue)
      if (dst.position() < dst.limit()) {
        read(dst, newBufferPosition)
      } else {
        newBufferPosition
      }
    }
  }

  private def xfadeNextLevelToBuffer(dst: ByteBuffer): Int = {
    if (dst.limit() - dst.position() < xfadeBuffer.limit() - xfadeBuffer.position()) {
      // The buffer is too small, switch to the next level without crossfading
      _currentLevel = _targetLevel
      currentLevelToBuffer(dst)
    } else {
      // Get some little data for current level
      val bufferPosition = looperPosition.get.add(-loopStart.toByte.bytePosition)
      xfadeBuffer.clear()
      mixLevelToBuffer(_currentLevel, currentLevelLimits, bufferPosition, xfadeBuffer)
      xfadeBuffer.position(0)

      // Get data for new level
      _currentLevel = _targetLevel
      currentLevelToBuffer(dst)
      dst.position(0)

      // Cross-fade from - to
      BufferMixer.xfade(audioFormat, xfadeBuffer, dst)
    }
  }

  private def currentLevelToBuffer(dst: ByteBuffer): Int = {
    currentLevelLimits = buffersForLevel(_currentLevel)
    mixLevelToBuffer(_currentLevel, currentLevelLimits, looperPosition.get.add(-loopStart.toByte.bytePosition), dst)
  }

  /**
   * @return New position in buffer.
   */
  private def mixLevelToBuffer(level: Int, levelLimits: (Int, Int), bufferPosition: BytePosition,
                               dst: ByteBuffer): Int = {
    val bytesCopied = copyAudioData(level, levelLimits, bufferPosition, dst)
    (bufferPosition.bytePosition + bytesCopied) % levelBuffers(currentLevelLimits._1).length
  }

  /**
   * @return Bytes copied.
   */
  private def copyAudioData(level: Int, levelLimits: (Int, Int), bufferPosition: BytePosition,
                            dst: ByteBuffer): Int = {
    if (level == levelLimits._1 || level == levelLimits._2) {
      val bytesCopied = directCopyAudioData(level, bufferPosition, dst)
      dst.position(dst.position() + bytesCopied)
      bytesCopied
    } else {
      mixAudioData(level, levelLimits, bufferPosition, dst)
    }
  }

  /**
   * @return Bytes mixed
   */
  private def mixAudioData(level: Int, levelLimits: (Int, Int), bufferPosition: BytePosition, dst: ByteBuffer): Int = {
    val diff = Math.abs(levelLimits._1 - levelLimits._2).toFloat
    val track1Level = (diff - Math.abs(levelLimits._1 - level)) / diff
    val track2Level = (diff - Math.abs(levelLimits._2 - level)) / diff

    val lowerLimitBuffer = ByteBuffer.wrap(levelBuffers(currentLevelLimits._1))
    val upperLimitBuffer = ByteBuffer.wrap(levelBuffers(currentLevelLimits._2))
    lowerLimitBuffer.position(bufferPosition.bytePosition)
    upperLimitBuffer.position(bufferPosition.bytePosition)
    BufferMixer.mix(audioFormat, lowerLimitBuffer, upperLimitBuffer, track1Level, track2Level, dst)
  }

  private def directCopyAudioData(level: Int, bufferPosition: BytePosition, dst: ByteBuffer): Int = {
    val srcBuffer = levelBuffers(level)
    val srcBytesLeft = srcBuffer.length - bufferPosition.bytePosition
    val dstBytesLeft = dst.limit() - dst.position()
    val bytesToCopy = Math.min(srcBytesLeft, dstBytesLeft)
    System.arraycopy(srcBuffer, bufferPosition.bytePosition, dst.array(), dst.position(), bytesToCopy)
    bytesToCopy
  }

  private def buffersForLevel(level: Int): (Int, Int) = {
    levelBuffers.get(level) match {
      case Some(_) => (level, level)
      case None =>
        val closest = levelBuffers.keys.toList.sortBy(key => Math.abs(key - level))
        val first = closest.head
        val second = closest.tail.headOption.getOrElse(first)
        (first, second)
    }
  }

  private def setTargetLevel(newLevel: Int): Unit = synchronized {
    if (newLevel != _currentLevel) {
      val min = levelBuffers.keys.min
      if (newLevel < min) {
        _targetLevel = min
      } else {
        val max = levelBuffers.keys.max
        if (newLevel > max) {
          _targetLevel = max
        } else {
          _targetLevel = newLevel
        }
      }
    }
  }

  private def initLooperPosition(masterStreamPosition: BytePosition): Unit = {
    // Initialize looper position
    looperPosition = Some(masterStreamPosition)

    val loopEnd = SamplePosition(audioFormat, loop.endExclusive)
    if (masterStreamPosition.bytePosition < loopStart.toByte.bytePosition ||
      masterStreamPosition.bytePosition >= loopEnd.toByte.bytePosition) {
      throw new OstrostrojException(s"Position outside of the loop! [${masterStreamPosition.bytePosition}, " +
        s"${loopStart.toByte.bytePosition}, ${loopEnd.toByte.bytePosition}]")
    }
  }
}

object LoopLooper {
  def apply(loop: Loop, audioFormat: AudioFormat): LoopLooper = {
    new LoopLooper(loop, audioFormat, loadLevelBuffers(loop, audioFormat))
  }

  /**
   * Loads all audio files for the loop into buffers. This can take longer therefore it's executed asynchronously by
   * the caller (PlaylistPlayer).
   */
  private def loadLevelBuffers(loop: Loop, audioFormat: AudioFormat): Map[Int, Array[Byte]] = {
    val bufferSize = (loop.endExclusive - loop.start)*audioFormat.getChannels*audioFormat.getSampleSizeInBits/8
    loop.tracks.map { track =>
      val stream = AudioSystem.getAudioInputStream(track.path.toFile)
      try {
        val buffer = new Array[Byte](bufferSize)
        val bytesRead = stream.read(buffer)
        if (bytesRead != bufferSize) {
          throw new OstrostrojException(s"Loop track reading problem! [$bufferSize, $bytesRead, ${track.path}]")
        }
        track.level -> buffer
      } finally {
        stream.close()
      }
    }.toMap
  }
}