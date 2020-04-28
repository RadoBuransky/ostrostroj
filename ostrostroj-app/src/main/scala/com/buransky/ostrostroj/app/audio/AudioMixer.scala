package com.buransky.ostrostroj.app.audio

object AudioMixer {
  /**
   * Mixes 16-bit, little-endian tracks onto the first provided track.
   * https://wiki.audacityteam.org/wiki/HowAudacityWorks#Audio_Mixing
   *
   * @param tracks Tracks to mix. The first item will contain the result.
   * @param length Number of bytes in each buffer.
   */
  def mix16bitLe(tracks: Seq[Array[Byte]], length: Int): Unit = {
    val acc = tracks.head
    for (sample <- 0 until (length / 2)) {
      var mix: Long = 0
      for (track <- tracks.indices) {
        mix += getShort(tracks(track), 2*sample)
      }

      if (mix > Short.MaxValue)
        mix = Short.MaxValue
      else {
        if (mix < Short.MinValue)
          mix = Short.MinValue
      }

      updateShort(acc, 2*sample, mix.toShort)
    }
  }

  private def updateShort(buffer: Array[Byte], pos: Int, s: Short): Unit = {
    val (hiByte, loByte) = shortToBytes(s)
    buffer.update(pos, hiByte)
    buffer.update(pos + 1, loByte)
  }
  private def shortToBytes(s: Short): (Byte, Byte) = ((s & 0xFF).toByte, ((s >>> 8) & 0xFF).toByte)

  private def getShort(buffer: Array[Byte], pos: Int): Short = bytesToShort(buffer(pos), buffer(pos + 1))
  private def bytesToShort(hiByte: Byte, loByte: Byte): Short = (((loByte & 0xFF) << 8) | (hiByte & 0xFF)).toShort
}
