package com.buransky.ostrostroj.driver

import java.io.RandomAccessFile
import java.nio.MappedByteBuffer
import java.nio.channels.FileChannel

/**
 * GPIO driver for Odroid C2 using memory mapped file. Supports digital IO only.
 *
 * @see - https://github.com/mattjlewis/diozero/blob/51b40dd76b64773dc8f23199b35ae49f785c940b/diozero-core/src/main/java/com/diozero/internal/board/odroid/OdroidC2MmapGpio.java
 *      - https://github.com/mattjlewis/diozero/blob/51b40dd76b64773dc8f23199b35ae49f785c940b/system-utils-native/src/main/c/com_diozero_util_MmapBufferNative.c
 */
private[driver] class MmapGpioDriver extends GpioDriver {
  import MmapGpioDriver._

  private val fd: RandomAccessFile = new RandomAccessFile(MEM_DEVICE, FILE_MODE)
  private val fc: FileChannel = fd.getChannel
  private val mem: MappedByteBuffer = fc.map(FileChannel.MapMode.READ_WRITE, GPIO_BASE_OFFSET, BLOCK_SIZE)

  override def pins: Seq[GpioPin] = {???}

  override def close(): Unit = {
    fc.close()
    fd.close()
  }
}

object MmapGpioDriver {
  private val MEM_DEVICE = "/dev/mem"
  private val FILE_MODE = "rws"
  private val GPIO_BASE_OFFSET: Long = 0xC8834000L
  private val BLOCK_SIZE: Long = 4*1024L

  def apply(): GpioDriver = new MmapGpioDriver()
}