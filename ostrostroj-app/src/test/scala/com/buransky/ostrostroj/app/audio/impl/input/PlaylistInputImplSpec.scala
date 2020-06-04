package com.buransky.ostrostroj.app.audio.impl.input

import com.buransky.ostrostroj.app.audio.{AudioBuffer, PlaylistStatus, SongInput, SongStatus}
import com.buransky.ostrostroj.app.show.{Playlist, Song}
import javax.sound.sampled.spi.AudioFileReader
import org.junit.runner.RunWith
import org.mockito.Mockito._
import org.scalatest.flatspec.AnyFlatSpec
import org.scalatestplus.junit.JUnitRunner
import org.scalatestplus.mockito.MockitoSugar

@RunWith(classOf[JUnitRunner])
class PlaylistInputImplSpec extends AnyFlatSpec with MockitoSugar {
  behavior of "read"

  it should "call song input to read" in {
    // Prepare
    val playlist = mock[Playlist]
    val songs = List(mock[Song])
    val songInput = mock[SongInput]
    def songInputFactory(song: Song, audioFileReader: AudioFileReader): SongInput = songInput
    val audioFileReader = mock[AudioFileReader]
    val emptyBuffer = mock[AudioBuffer]
    val fullBuffer = mock[AudioBuffer]
    when(songInput.read(emptyBuffer)).thenReturn(fullBuffer)
    when(fullBuffer.endOfStream).thenReturn(false)
    when(playlist.songs).thenReturn(songs)
    val playlistInput = new PlaylistInputImpl(playlist, songInputFactory, audioFileReader)

    // Execute
    val result = playlistInput.read(emptyBuffer)

    // Assert & verify
    assert(result == fullBuffer)
    verify(playlist).songs
    verify(songInput).read(emptyBuffer)
    verify(fullBuffer).endOfStream
    verifyNoMoreInteractions(audioFileReader)
    verifyNoMoreInteractions(songInput)
    verifyNoMoreInteractions(emptyBuffer)
    verifyNoMoreInteractions(fullBuffer)
  }

  it should "switch to the next song after reading last data" in {
    // Prepare
    val playlist = mock[Playlist]
    val song1 = mock[Song]
    val song2 = mock[Song]
    val songs = List(song1, song2)
    val songInput1 = mock[SongInput]
    val songInput2 = mock[SongInput]
    def songInputFactory(song: Song, audioFileReader: AudioFileReader): SongInput = song match {
      case `song1` => songInput1
      case `song2` => songInput2
    }
    val audioFileReader = mock[AudioFileReader]
    val emptyBuffer = mock[AudioBuffer]
    val fullBuffer1 = mock[AudioBuffer]
    val fullBuffer2 = mock[AudioBuffer]
    when(songInput1.read(emptyBuffer)).thenReturn(fullBuffer1)
    when(songInput2.read(emptyBuffer)).thenReturn(fullBuffer2)
    when(fullBuffer1.endOfStream).thenReturn(true)
    when(fullBuffer2.endOfStream).thenReturn(false)
    when(playlist.songs).thenReturn(songs)
    val playlistInput = new PlaylistInputImpl(playlist, songInputFactory, audioFileReader)

    // Step 1: read from song 1
    val result1 = playlistInput.read(emptyBuffer)
    assert(result1 == fullBuffer1)
    verify(songInput1).read(emptyBuffer)
    verify(songInput1).close()
    verify(fullBuffer1).endOfStream
    verifyZeroInteractions(songInput2)

    // Step 2: read from song 2
    val result2 = playlistInput.read(emptyBuffer)
    assert(result2 == fullBuffer2)
    verify(songInput2).read(emptyBuffer)

    verify(playlist, times(3)).songs
    verifyNoMoreInteractions(audioFileReader)
    verifyNoMoreInteractions(songInput1)
    verifyNoMoreInteractions(songInput2)
    verifyNoMoreInteractions(emptyBuffer)
    verifyNoMoreInteractions(fullBuffer1)
  }

  behavior of "close"

  it should "close current song input" in {
    // Prepare
    val playlist = mock[Playlist]
    val songs = List(mock[Song])
    val songInput = mock[SongInput]
    def songInputFactory(song: Song, audioFileReader: AudioFileReader): SongInput = songInput
    val audioFileReader = mock[AudioFileReader]
    when(playlist.songs).thenReturn(songs)
    val playlistInput = new PlaylistInputImpl(playlist, songInputFactory, audioFileReader)

    // Execute
    playlistInput.close()

    // Assert & verify
    verify(songInput).close()
    verify(playlist).songs
    verifyNoMoreInteractions(audioFileReader)
    verifyNoMoreInteractions(songInput)
  }

  behavior of "status"

  it should "call song input for status" in {
    // Prepare
    val playlist = mock[Playlist]
    val songs = List(mock[Song])
    val songInput = mock[SongInput]
    def songInputFactory(song: Song, audioFileReader: AudioFileReader): SongInput = songInput
    val audioFileReader = mock[AudioFileReader]
    when(playlist.songs).thenReturn(songs)
    val playlistInput = new PlaylistInputImpl(playlist, songInputFactory, audioFileReader)
    val songStatus = mock[SongStatus]
    when(songInput.status).thenReturn(songStatus)

    // Execute
    val result = playlistInput.status

    // Assert & verify
    assert(result == PlaylistStatus(songStatus))
    verify(songInput).status
    verify(playlist).songs
    verifyNoMoreInteractions(audioFileReader)
    verifyNoMoreInteractions(songInput)
  }
}