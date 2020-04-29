package com.buransky.ostrostroj.app.player

import java.io.InputStream
import java.nio.ByteBuffer
import java.util.concurrent.{Executors, Semaphore}

import org.slf4j.LoggerFactory

import scala.collection.mutable
import scala.concurrent.{ExecutionContext, Future}

class BufferedInputStream(upstream: InputStream) extends AutoCloseable {
  import BufferedInputStream._
  private val executorService = Executors.newSingleThreadExecutor()
  private implicit val executionContext: ExecutionContext = ExecutionContext.fromExecutorService(executorService)
  private val buffers: mutable.Queue[Future[ByteBuffer]] = mutable.Queue.empty
  private val dequeueSemaphore: Semaphore = new Semaphore(buffersMaxCount)

  // Start buffering immediately
  enqueueNextDataReader()

  /**
   * The caller must wait for the returned Future to complete before calling this method again.
   */
  def byteBuffer: Future[ByteBuffer] = {
    val result = synchronized {
       if (buffers.isEmpty) {
        endOfStreamReader
      } else {
        buffers.dequeue()
      }
    }
    dequeueSemaphore.release()
    if (logger.isDebugEnabled) {
      logger.debug(s"Dequeue. [${buffers.length}]")
    }
    result
  }

  private def enqueueNextDataReader(): Unit = {
    logger.trace("Acquiring permit...")
    dequeueSemaphore.acquire()
    logger.trace("Permit acquired.")
    synchronized {
      val nextDataReader = Future {
        val bytesArray = new Array[Byte](bufferSize)
        val bytesRead = upstream.read(bytesArray)
        if (bytesRead == -1) {
          logger.debug(s"End of stream reached.")
          endOfStreamResult
        } else {
          logger.debug(s"Bytes read. [$bytesRead]")
          val result = ByteBuffer.wrap(bytesArray, 0, bytesRead)
          enqueueNextDataReader()
          result
        }
      }
      buffers.enqueue(nextDataReader)
      if (logger.isDebugEnabled) {
        logger.debug(s"Enqueue. [${buffers.length}]")
      }
    }
  }

  override def close(): Unit = {
    executorService.shutdownNow()
  }
}

object BufferedInputStream {
  private val logger = LoggerFactory.getLogger(classOf[BufferedInputStream])
  private val endOfStreamResult = ByteBuffer.allocate(0)
  private val endOfStreamReader = Future.successful(endOfStreamResult)
  private val buffersMaxCount = 15
  private val bufferSize = 16*4096
}