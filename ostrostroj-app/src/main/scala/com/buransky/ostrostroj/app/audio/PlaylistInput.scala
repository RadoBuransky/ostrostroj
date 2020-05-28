package com.buransky.ostrostroj.app.audio

import com.buransky.ostrostroj.app.audio.impl.AsyncListeningPlaylistInput

private[audio] trait PlaylistInput extends AutoCloseable {
  def run(): Unit
}

private[audio] object PlaylistInput {
  def apply(initSong: Int, javaSoundOutput: JavaSoundOutput): PlaylistInput = new AsyncListeningPlaylistInput(initSong,
    javaSoundOutput)
}