package com.buransky.ostrostroj.app.audio

private[audio] trait AudioOutput extends AutoCloseable {
  def start(): Unit
  def stop(): Unit

  def write(): FrameCount

  def queueFull(buffer: AudioBuffer): Unit
  def dequeueEmpty(): AudioBuffer
  def tryDequeueEmpty(): Option[AudioBuffer]

  def volumeUp(): Double
  def volumeDown(): Double
  def volume: Double

  def framesBuffered: FrameCount
}