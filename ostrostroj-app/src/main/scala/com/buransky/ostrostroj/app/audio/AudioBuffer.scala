package com.buransky.ostrostroj.app.audio

import java.nio.file.Path

import javax.sound.sampled.AudioFormat
import javax.sound.sampled.spi.AudioFileReader
import org.slf4j.LoggerFactory

/**
 * Typed wrapper for values measured in number of audio frames.
 * @param value Number of audio frames.
 */
final case class FrameCount(value: Int) extends AnyVal {
  def +(other: FrameCount): FrameCount = FrameCount(value + other.value)
  def -(other: FrameCount): FrameCount = FrameCount(value - other.value)
}
/**
 * Buffer for audio data.
 */
case class AudioBuffer(byteArray: Array[Byte],
                       frameSize: Int,
                       position: FrameCount,
                       limit: FrameCount,
                       endOfStream: Boolean) {
  def bytePosition: Int = position.value*frameSize
  def byteLimit: Int = limit.value*frameSize
  def byteSize: Int = byteLimit - bytePosition
  def size: FrameCount = FrameCount(limit.value - position.value)
}

object AudioBuffer {
  private val logger = LoggerFactory.getLogger(classOf[AudioBuffer])

  def apply(audioFormat: AudioFormat, byteSize: Int): AudioBuffer = {
    val byteArray = new Array[Byte](byteSize)
    new AudioBuffer(byteArray, audioFormat.getFrameSize, FrameCount(0), FrameCount(0), endOfStream = true)
  }

  def apply(path: Path, audioFileReader: AudioFileReader): AudioBuffer = {
    val audioInputStream = audioFileReader.getAudioInputStream(path.toFile)
    try {
      val byteSize = audioInputStream.available()
      val result = AudioBuffer(audioInputStream.getFormat, byteSize)
      val bytesRead = audioInputStream.read(result.byteArray)
      if (bytesRead != byteSize) {
        logger.warn(s"Unexpected read size! [$byteSize, $bytesRead, $path]")
      } else {
        logger.debug(s"File loaded into memory buffer. [$bytesRead, $path]")
      }
      result.copy(limit = FrameCount(bytesRead/result.frameSize), endOfStream = true)
    } finally {
      audioInputStream.close()
    }
  }
}