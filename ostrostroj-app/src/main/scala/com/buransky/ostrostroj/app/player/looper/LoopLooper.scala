package com.buransky.ostrostroj.app.player.looper

import java.nio.ByteBuffer

import com.buransky.ostrostroj.app.common.OstrostrojException
import com.buransky.ostrostroj.app.player.{BytePosition, SamplePosition}
import com.buransky.ostrostroj.app.show.Loop
import javax.sound.sampled.{AudioFormat, AudioSystem}
import org.slf4j.LoggerFactory

class LoopLooper(val loop: Loop, audioFormat: AudioFormat, levelBuffers: Map[Int, Array[Byte]]) {
  import LoopLooper._

  private val loopStart = SamplePosition(audioFormat, loop.start)
  private val loopEnd = SamplePosition(audioFormat, loop.endExclusive)

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
  private var _counter: Int = 1

  /**
   * @return Number of bytes to skip in the master stream.
   */
  def fill(buffer: ByteBuffer, masterStreamPosition: BytePosition): BytePosition = synchronized {
    logger.trace(s"fill(ByteBuffer(${buffer.position()}, ${buffer.limit()}, ${buffer.capacity()}), " +
      s"${masterStreamPosition.bytePosition})")

    if (looperPosition.isEmpty) {
      initLooperPosition(masterStreamPosition)
    }

    val result = looperPosition match {
      case Some(lp) =>
        val bufferPosition = BytePosition(audioFormat, lp.bytePosition - loopStart.toByte.bytePosition)
        if (bufferPosition.bytePosition == 0 && _draining) {
          BytePosition(audioFormat, 0)
        } else {
          val dstBuffer = ByteBuffer.wrap(buffer.array())
          dstBuffer.position(buffer.limit())
          dstBuffer.limit(buffer.capacity())

          val newBufferPosition = read(dstBuffer, bufferPosition)

          buffer.limit(dstBuffer.position())
          createResult(masterStreamPosition)
        }
      case None => throw new OstrostrojException("Looper position unknown!")
    }
    logger.trace(s"fill = $result")
    result
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
  def counter: Int = _counter

  private def createResult(masterStreamPosition: BytePosition): BytePosition = {
    BytePosition(audioFormat, loopEnd.toByte.bytePosition - masterStreamPosition.bytePosition)
  }

  private def read(dst: ByteBuffer, bufferPosition: BytePosition): BytePosition = {
    logger.trace(s"read(ByteBuffer(${dst.position()}, ${dst.limit()}, ${dst.capacity()}), " +
      s"${bufferPosition.bytePosition})")
    val result = if (bufferPosition.bytePosition == 0 && _draining) {
      // Draining done, we're done with this loop
      bufferPosition
    } else {
      if (bufferPosition.bytePosition == 0) {
        _counter += 1
      }

      val newBufferPosition = if (_currentLevel == _targetLevel || bufferPosition.bytePosition != 0) {
        currentLevelToBuffer(dst)
      } else {
        // Crossfade only at the beginning of the loop
        xfadeNextLevelToBuffer(dst)
      }

      if (newBufferPosition.bytePosition == bufferPosition.bytePosition) {
        throw new OstrostrojException(s"No progress? [${newBufferPosition.bytePosition}," +
          s"${bufferPosition.bytePosition}]")
      }

      // Read until buffer is full or draining
      looperPosition = Some(newBufferPosition.add(loopStart.toByte.bytePosition))
      if (dst.position() < dst.limit()) {
        read(dst, newBufferPosition)
      } else {
        newBufferPosition
      }
    }
    logger.trace(s"read = $result")
    result
  }

  private def xfadeNextLevelToBuffer(dst: ByteBuffer): BytePosition = {
    logger.trace(s"xfadeNextLevelToBuffer(ByteBuffer(${dst.position()}, ${dst.limit()}, ${dst.capacity()}))")
    val result = if (dst.limit() - dst.position() < xfadeBuffer.limit() - xfadeBuffer.position()) {
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
      val result = currentLevelToBuffer(dst)
      dst.position(0)

      // Cross-fade from - to
      BufferMixer.xfade(audioFormat, xfadeBuffer, dst)
      result
    }
    logger.trace(s"xfadeNextLevelToBuffer = $result")
    result
  }

  private def currentLevelToBuffer(dst: ByteBuffer): BytePosition = {
    logger.trace(s"currentLevelToBuffer(ByteBuffer(${dst.position()}, ${dst.limit()}, ${dst.capacity()}))")
    currentLevelLimits = buffersForLevel(_currentLevel)
    val result = mixLevelToBuffer(_currentLevel, currentLevelLimits, looperPosition.get.add(
      -loopStart.toByte.bytePosition), dst)
    logger.trace(s"currentLevelToBuffer = $result")
    result
  }

  /**
   * @return New position in buffer.
   */
  private def mixLevelToBuffer(level: Int, levelLimits: (Int, Int), bufferPosition: BytePosition,
                               dst: ByteBuffer): BytePosition = {
    logger.trace(s"mixLevelToBuffer($level, $levelLimits, ${bufferPosition.bytePosition}, " +
      s"ByteBuffer(${dst.position()}, ${dst.limit()}, ${dst.capacity()}))")
    val bytesCopied = copyAudioData(level, levelLimits, bufferPosition, dst)
    val result = BytePosition(audioFormat, (bufferPosition.bytePosition + bytesCopied) %
      levelBuffers(currentLevelLimits._1).length)
    logger.trace(s"mixLevelToBuffer = $result")
    result
  }

  /**
   * @return Bytes copied.
   */
  private def copyAudioData(level: Int, levelLimits: (Int, Int), bufferPosition: BytePosition, dst: ByteBuffer): Int = {
    logger.trace(s"copyAudioData($level, $levelLimits, ${bufferPosition.bytePosition}, " +
      s"ByteBuffer(${dst.position()}, ${dst.limit()}, ${dst.capacity()}))")
    val result = if (level == levelLimits._1 || level == levelLimits._2) {
      val bytesCopied = directCopyAudioData(level, bufferPosition, dst)
      dst.position(dst.position() + bytesCopied)
      bytesCopied
    } else {
      mixAudioData(level, levelLimits, bufferPosition, dst)
    }
    logger.trace(s"copyAudioData = $result")
    result
  }

  /**
   * @return Bytes mixed
   */
  private def mixAudioData(level: Int, levelLimits: (Int, Int), bufferPosition: BytePosition, dst: ByteBuffer): Int = {
    logger.trace(s"mixAudioData($level, $levelLimits, ${bufferPosition.bytePosition}, " +
      s"ByteBuffer(${dst.position()}, ${dst.limit()}, ${dst.capacity()}))")

    val diff = Math.abs(levelLimits._1 - levelLimits._2).toFloat
    val track1Level = (diff - Math.abs(levelLimits._1 - level)) / diff
    val track2Level = (diff - Math.abs(levelLimits._2 - level)) / diff

    val dstSize = dst.limit() - dst.position()
    val remainingSize = levelBuffers.head._2.length - bufferPosition.bytePosition
    val sizeToMix = Math.min(dstSize, remainingSize)

    val lowerLimitBuffer = ByteBuffer.wrap(levelBuffers(currentLevelLimits._1))
    lowerLimitBuffer.position(bufferPosition.bytePosition)
    lowerLimitBuffer.limit(bufferPosition.bytePosition + sizeToMix)
    val upperLimitBuffer = ByteBuffer.wrap(levelBuffers(currentLevelLimits._2))
    upperLimitBuffer.position(bufferPosition.bytePosition)
    upperLimitBuffer.limit(bufferPosition.bytePosition + sizeToMix)
    val result = BufferMixer.mix(audioFormat, lowerLimitBuffer, upperLimitBuffer, track1Level, track2Level, dst)
    logger.trace(s"mixAudioData = $result")
    result
  }

  private def directCopyAudioData(level: Int, bufferPosition: BytePosition, dst: ByteBuffer): Int = {
    logger.trace(s"directCopyAudioData($level, ${bufferPosition.bytePosition}, " +
      s"ByteBuffer(${dst.position()}, ${dst.limit()}, ${dst.capacity()}))")
    val srcBuffer = levelBuffers(level)
    val srcBytesLeft = srcBuffer.length - bufferPosition.bytePosition
    val dstBytesLeft = dst.limit() - dst.position()
    val bytesToCopy = Math.min(srcBytesLeft, dstBytesLeft)
    System.arraycopy(srcBuffer, bufferPosition.bytePosition, dst.array(), dst.position(), bytesToCopy)
    logger.trace(s"directCopyAudioData = $bytesToCopy")
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

    if (masterStreamPosition.bytePosition < loopStart.toByte.bytePosition ||
      masterStreamPosition.bytePosition >= loopEnd.toByte.bytePosition) {
      throw new OstrostrojException(s"Position outside of the loop! [${masterStreamPosition.bytePosition}, " +
        s"${loopStart.toByte.bytePosition}, ${loopEnd.toByte.bytePosition}]")
    }
  }
}

object LoopLooper {
  private val logger = LoggerFactory.getLogger(classOf[LoopLooper])

  def apply(loop: Loop, audioFormat: AudioFormat): LoopLooper = {
    new LoopLooper(loop, audioFormat, loadLevelBuffers(loop, audioFormat))
  }

  /**
   * Loads all audio files for the loop into buffers. This can take longer therefore it's executed asynchronously by
   * the caller (PlaylistPlayer).
   */
  private def loadLevelBuffers(loop: Loop, audioFormat: AudioFormat): Map[Int, Array[Byte]] = {
    val bufferSize = SamplePosition(audioFormat, loop.endExclusive - loop.start).toByte.bytePosition
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