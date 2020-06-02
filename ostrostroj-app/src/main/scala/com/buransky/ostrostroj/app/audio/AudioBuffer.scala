package com.buransky.ostrostroj.app.audio

import java.nio.file.Path

import com.buransky.ostrostroj.app.common.OstrostrojException
import javax.sound.sampled.spi.AudioFileReader
import org.slf4j.LoggerFactory

/**
 * Typed wrapper for values measured in number of audio frames.
 * @param value Number of audio frames.
 */
final case class FrameCount(value: Int) extends AnyVal

sealed abstract class BitsPerSample(val bits: Int) {
  def bytes: Int = bits/8
}
case object SixteenBits extends BitsPerSample(16)
case object EightBits extends BitsPerSample(8)

object BitsPerSample {
  def apply(bitsPerSample: Int): BitsPerSample = {
    bitsPerSample match {
      case 8 => EightBits
      case 16 => SixteenBits
      case other => throw new OstrostrojException(s"Unexpected number of bits! [$other]")
    }
  }
}

/**
 * Buffer for audio data.
 */
class AudioBuffer(val byteArray: Array[Byte],
                  val channels: Int,
                  val bitsPerSample: BitsPerSample,
                  val position: FrameCount,
                  val limit: FrameCount,
                  val endOfStream: Boolean) {
  def bytePosition: Int = position.value*channels*bitsPerSample.bytes
  def byteLimit: Int = limit.value*channels*bitsPerSample.bytes
  def byteSize: Int = byteLimit - bytePosition
  def size: FrameCount = FrameCount(limit.value - position.value)
}

object AudioBuffer {
  private val logger = LoggerFactory.getLogger(classOf[AudioBuffer])

  def apply(path: Path, audioFileReader: AudioFileReader): AudioBuffer = {
    val audioInputStream = audioFileReader.getAudioInputStream(path.toFile)
    try {
      val byteSize = audioInputStream.available()
      val byteArray = new Array[Byte](byteSize)
      val bytesRead = audioInputStream.read(byteArray)
      if (bytesRead != byteSize) {
        logger.warn(s"Unexpected read size! [$byteSize, $bytesRead, $path]")
      } else {
        logger.debug(s"File loaded into memory buffer. [$bytesRead, $path]")
      }
      val audioFormat = audioInputStream.getFormat
      new AudioBuffer(byteArray, audioFormat.getChannels, BitsPerSample(audioFormat.getSampleSizeInBits), FrameCount(0),
        FrameCount(bytesRead/audioFormat.getFrameSize), true)
    } finally {
      audioInputStream.close()
    }
  }
}