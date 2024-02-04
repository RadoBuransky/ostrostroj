package com.buransky.ostrostroj.player.midi

trait MidiCommands {
  def start(): Unit
  def stop(): Unit
  def continue(): Unit
  def programChange(num: Int): Unit
  def tick(): Unit
}
