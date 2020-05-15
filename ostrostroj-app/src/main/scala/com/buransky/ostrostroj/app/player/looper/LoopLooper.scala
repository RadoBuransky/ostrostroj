package com.buransky.ostrostroj.app.player.looper

import java.nio.ByteBuffer

import com.buransky.ostrostroj.app.common.OstrostrojException
import com.buransky.ostrostroj.app.show.Loop
import javax.sound.sampled.{AudioFormat, AudioSystem}

import scala.annotation.tailrec

case class LooperReadResult(bytesRead: Int, masterSkip: Int)
case object LooperReadResult {
  val empty: LooperReadResult = LooperReadResult(-1, 0)
}

class LoopLooper(loop: Loop, audioFormat: AudioFormat, levelBuffers: Map[Int, Array[Byte]]) {
  /**
   * Small buffer used only for crossfading between levels.
   */
  private val xfadeBuffer: ByteBuffer = ByteBuffer.allocate(BufferMixer.xfadingBufferLength(audioFormat))

  /**
   * Current loop level index.
   */
  private var currentLevel: Int = 0

  /**
   * Lower and higher limits for the current level.
   */
  private var currentLevelLimits: (Int, Int) = (0, 0)

  /**
   * Target level to get to
   */
  private var targetLevel: Int = 0
  private var looperPosition: Int = -1
  private var draining: Boolean = false

  def read(dst: Array[Byte], masterStreamPosition: Int): LooperReadResult = synchronized {
    if (looperPosition == -1) {
      initLooperPosition(masterStreamPosition)
    }

    val bufferPosition = looperPosition - loop.start
    if (bufferPosition == 0 && draining) {
      LooperReadResult(-1, 0)
    } else {
      val dstBuffer = ByteBuffer.wrap(dst)
      val newBufferPosition = read(dstBuffer, bufferPosition)
      looperPosition = loop.start + newBufferPosition
      createResult(masterStreamPosition, dstBuffer)
    }
  }

  def harder(): Unit = setTargetLevel(targetLevel + 1)
  def softer(): Unit = setTargetLevel(targetLevel - 1)
  def startDraining(): Unit = synchronized { draining = true }
  def stopDraining(): Unit = synchronized { draining = false }

  private def createResult(masterStreamPosition: Int, dstBuffer: ByteBuffer): LooperReadResult = {
    val loopEndPosition = loop.endExclusive * audioFormat.getChannels * audioFormat.getSampleSizeInBits / 8
    LooperReadResult(dstBuffer.position(), loopEndPosition - masterStreamPosition)
  }

  @tailrec
  private def read(dst: ByteBuffer, bufferPosition: Int): Int = {
    if (bufferPosition == 0 && draining) {
      // Draining done, we're done with this loop
      bufferPosition
    } else {
      val newBufferPosition = if (currentLevel == targetLevel || bufferPosition != 0) {
        currentLevelToBuffer(dst)
      } else {
        // Crossfade only at the beginning of the loop
        xfadeNextLevelToBuffer(dst)
      }

      // Read until buffer is full or draining
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
      currentLevel = targetLevel
      currentLevelToBuffer(dst)
    } else {
      // Get some little data for current level
      val bufferPosition = looperPosition - loop.start
      xfadeBuffer.clear()
      mixLevelToBuffer(currentLevel, currentLevelLimits, bufferPosition, xfadeBuffer)
      xfadeBuffer.position(0)

      // Get data for new level
      currentLevel = targetLevel
      currentLevelToBuffer(dst)
      dst.position(0)

      // Cross-fade from - to
      BufferMixer.xfade(audioFormat, xfadeBuffer, dst)
    }
  }

  private def currentLevelToBuffer(dst: ByteBuffer) = {
    currentLevelLimits = buffersForLevel(currentLevel)
    mixLevelToBuffer(currentLevel, currentLevelLimits, looperPosition - loop.start, dst)
  }

  /**
   * @return New position in buffer.
   */
  private def mixLevelToBuffer(level: Int, levelLimits: (Int, Int), bufferPosition: Int, dst: ByteBuffer): Int = {
    val bytesCopied = copyAudioData(level, levelLimits, bufferPosition, dst)
    (bufferPosition + bytesCopied) % levelBuffers(currentLevelLimits._1).length
  }

  /**
   * @return Bytes copied.
   */
  private def copyAudioData(level: Int, levelLimits: (Int, Int), bufferPosition: Int, dst: ByteBuffer): Int = {
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
  private def mixAudioData(level: Int, levelLimits: (Int, Int), bufferPosition: Int, dst: ByteBuffer): Int = {
    val diff = Math.abs(levelLimits._1 - levelLimits._2).toFloat
    val track1Level = (diff - Math.abs(levelLimits._1 - level)) / diff
    val track2Level = (diff - Math.abs(levelLimits._2 - level)) / diff

    val lowerLimitBuffer = ByteBuffer.wrap(levelBuffers(currentLevelLimits._1))
    val upperLimitBuffer = ByteBuffer.wrap(levelBuffers(currentLevelLimits._2))
    lowerLimitBuffer.position(bufferPosition)
    upperLimitBuffer.position(bufferPosition)
    BufferMixer.mix(audioFormat, lowerLimitBuffer, upperLimitBuffer, track1Level, track2Level, dst)
  }

  private def directCopyAudioData(level: Int, bufferPosition: Int, dst: ByteBuffer) = {
    val srcBuffer = levelBuffers(level)
    val srcBytesLeft = srcBuffer.length - bufferPosition
    val dstBytesLeft = dst.limit() - dst.position()
    val bytesToCopy = Math.min(srcBytesLeft, dstBytesLeft)
    System.arraycopy(srcBuffer, bufferPosition, dst.array(), dst.position(), bytesToCopy)
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
    if (newLevel != currentLevel) {
      val min = levelBuffers.keys.min
      if (newLevel < min) {
        targetLevel = min
      } else {
        val max = levelBuffers.keys.max
        if (newLevel > max) {
          targetLevel = max
        } else {
          targetLevel = newLevel
        }
      }
    }
  }

  private def initLooperPosition(masterStreamPosition: Int): Unit = {
    // Initialize looper position
    looperPosition = masterStreamPosition

    if (looperPosition < loop.start || looperPosition > loop.endExclusive) {
      throw new OstrostrojException(s"Position outside of the loop! [$looperPosition, ${loop.start}, " +
        s"${loop.endExclusive}]")
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