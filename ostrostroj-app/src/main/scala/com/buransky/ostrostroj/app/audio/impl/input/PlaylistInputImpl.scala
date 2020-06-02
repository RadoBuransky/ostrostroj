package com.buransky.ostrostroj.app.audio.impl.input

import com.buransky.ostrostroj.app.audio.{AudioBuffer, AudioEvent, PlaylistInput, SongInput}
import com.buransky.ostrostroj.app.show.{Playlist, Song}
import org.slf4j.LoggerFactory

private[audio] class PlaylistInputImpl(playlist: Playlist,
                                       songInputFactory: (Song) => SongInput) extends PlaylistInput {
  import PlaylistInputImpl._
  private var songIndex = 0
  private var _songInput: SongInput = createSongInput(songIndex)

  override def songInput: SongInput = synchronized { _songInput }
  override def read(buffer: AudioBuffer): AudioEvent = synchronized {
    val result = songInput.read(buffer)
    if (result.endOfStream) {
      logger.debug("End of stream.")
      if (songIndex < playlist.songs.length - 1) {
        songIndex += 1
        _songInput = createSongInput(songIndex)
      }
    }
    result
  }

  override def close(): Unit = synchronized {
    _songInput.close()
    logger.debug("Playlist input closed.")
  }

  private def createSongInput(songIndex: Int): SongInput = {
    if (_songInput != null) {
      _songInput.close()
    }

    logger.debug(s"Creating song input... [$songIndex]")
    songInputFactory(playlist.songs(songIndex))
  }
}

private object PlaylistInputImpl {
  private val logger = LoggerFactory.getLogger(classOf[PlaylistInputImpl])
}