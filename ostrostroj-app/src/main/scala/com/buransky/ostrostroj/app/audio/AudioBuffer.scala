package com.buransky.ostrostroj.app.audio

import java.nio.file.Path

import com.google.common.base.Preconditions._
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
 * @param byteArray Actual audio data buffer.
 * @param frameSize Size of single audio frame in bytes (4 for 16-bit stereo)
 * @param position Start of audio data in the buffer (in number of frames).
 * @param limit End of audio data (exclusive) in the buffer (in number of frames).
 * @param endOfStream true if this is the last data available in the source, false otherwise.
 */
case class AudioBuffer(byteArray: Array[Byte],
                       frameSize: Int,
                       position: FrameCount,
                       limit: FrameCount,
                       endOfStream: Boolean) {
  checkArgument(byteArray.length > 0)
  checkArgument(frameSize == 2 || frameSize == 4)
  checkArgument(position.value >= 0)
  checkArgument(limit.value >= 0)
  checkArgument(limit.value >= position.value)

  def size: FrameCount = FrameCount(limit.value - position.value)
  def capacity: FrameCount = FrameCount(byteArray.length / frameSize)
  def bytePosition: Int = position.value*frameSize
  def byteLimit: Int = limit.value*frameSize
  def byteSize: Int = byteLimit - bytePosition
  def byteCapacity: Int = byteArray.length
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