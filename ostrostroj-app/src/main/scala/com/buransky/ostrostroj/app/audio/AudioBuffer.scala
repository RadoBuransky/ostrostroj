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
                       channels: Int,
                       position: FrameCount,
                       limit: FrameCount,
                       endOfStream: Boolean) {
  checkArgument(byteArray.length > 0, "byteArray is empty.".asInstanceOf[AnyRef])
  checkArgument(frameSize == 2 || frameSize == 4, s"Unsupported frameSize. [$frameSize]".asInstanceOf[AnyRef])
  checkArgument(channels == 1 || channels == 2, s"Unsupported channels. [$channels]".asInstanceOf[AnyRef])
  checkArgument(position.value >= 0, s"Negative position. [${position.value}]".asInstanceOf[AnyRef])
  checkArgument(limit.value >= 0, s"Negative limit. [${limit.value}]".asInstanceOf[AnyRef])
  checkArgument(limit.value >= position.value, (s"Limit smaller than position. " +
    s"[${position.value}, ${limit.value}]").asInstanceOf[AnyRef])
  checkArgument(position.value < capacity.value, (s"Position outside of byteBuffer. " +
    s"[${position.value}, ${capacity.value}]").asInstanceOf[AnyRef])
  checkArgument(limit.value <= capacity.value, (s"Limit outside of byteBuffer. " +
    s"[${limit.value}, ${capacity.value}]").asInstanceOf[AnyRef])

  def size: FrameCount = FrameCount(limit.value - position.value)
  def capacity: FrameCount = FrameCount(byteArray.length / frameSize)
  def bytePosition: Int = position.value*frameSize
  def byteLimit: Int = limit.value*frameSize+1
  def byteSize: Int = (limit-position).value*frameSize
  def byteCapacity: Int = byteArray.length

  def putLeShort(s: Short, position: FrameCount, channel: Int): Unit = {
    val (loByte, hiByte) = shortToLeBytes(s)
    val samplePosition = frameChannelToSample(position, channel)
    byteArray.update(samplePosition, loByte)
    byteArray.update(samplePosition + 1, hiByte)
  }

  def readLeShort(position: FrameCount, channel: Int): Short = {
    val samplePosition = frameChannelToSample(position, channel)
    val loByte = byteArray(samplePosition)
    val hiByte = byteArray(samplePosition + 1)
    leBytesToShort(loByte, hiByte)
  }

  private def shortToLeBytes(s: Short): (Byte, Byte) = ((s & 0xFF).toByte, ((s >>> 8) & 0xFF).toByte)
  private def leBytesToShort(loByte: Byte, hiByte: Byte): Short = (((hiByte & 0xFF) << 8) | (loByte & 0xFF)).toShort
  private def frameChannelToSample(position: FrameCount, channel: Int): Int =
    position.value*frameSize + channel*(frameSize/channels)
}

object AudioBuffer {
  private val logger = LoggerFactory.getLogger(classOf[AudioBuffer])

  /**
   * Creates empty audio buffer.
   * @param audioFormat Audio format of the buffer.
   * @param byteSize Byte size of the buffer. Must be aligned to frame size of the format.
   */
  def apply(audioFormat: AudioFormat, byteSize: Int): AudioBuffer = {
    checkArgument(byteSize%audioFormat.getFrameSize == 0, (s"Illegal byteSize. " +
      s"[$byteSize, ${audioFormat.getFrameSize}]").asInstanceOf[AnyRef])
    val byteArray = new Array[Byte](byteSize)
    new AudioBuffer(byteArray, audioFormat.getFrameSize, audioFormat.getChannels, FrameCount(0), FrameCount(0),
      endOfStream = false)
  }

  /**
   * Load the whole audio file located at the provided path to audio buffer.
   * @param path Audio file to load.
   * @param audioFileReader Reader to use to load the file.
   */
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