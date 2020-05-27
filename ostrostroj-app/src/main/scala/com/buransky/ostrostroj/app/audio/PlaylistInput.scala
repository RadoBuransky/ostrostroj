package com.buransky.ostrostroj.app.audio

import com.buransky.ostrostroj.app.audio.impl.ListeningPlaylistInputThread

private[audio] trait PlaylistInput {
}

private[audio] object PlaylistInput {
  def apply(initSong: Int, javaSoundOutput: JavaSoundOutput): PlaylistInput = new ListeningPlaylistInputThread(initSong,
    javaSoundOutput)
}