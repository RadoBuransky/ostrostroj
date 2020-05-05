package com.buransky.ostrostroj.app.player.looper

import com.buransky.ostrostroj.app.common.OstrostrojException
import com.buransky.ostrostroj.app.show.{Loop, Track}
import javax.sound.sampled.{AudioFormat, AudioSystem}

case class LooperReadResult(bytesRead: Int, masterSkip: Int)

class LoopLooper(loop: Loop, audioFormat: AudioFormat) {
  private val levelBuffers: Map[Int, Array[Byte]] = loadLevelBuffers(loop.tracks)
  private var currentLevel: Int = 0
  private var currentBuffers: (Int, Int) = (0, 0)
  private var targetLevel: Int = 0
  private var looperPosition: Int = -1
  private var draining: Boolean = false

  def read(buffer: Array[Byte], masterStreamPosition: Int): LooperReadResult = synchronized {
    ???
  }
  def harder(): Unit = setTargetLevel(targetLevel + 1)
  def softer(): Unit = setTargetLevel(targetLevel - 1)
  def startDraining(): Unit = synchronized { draining = true }
  def stopDraining(): Unit = synchronized { draining = false }

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
  private def loadLevelBuffers(tracks: Seq[Track]): Map[Int, Array[Byte]] = {
    val bufferSize = (loop.endExclusive - loop.start)*audioFormat.getChannels*audioFormat.getSampleSizeInBits/8
    tracks.map { track =>
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
