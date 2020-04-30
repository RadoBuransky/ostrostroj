package com.buransky.ostrostroj.app.show

import java.nio.file.Paths

import org.junit.runner.RunWith
import org.scalatest.flatspec.AnyFlatSpec
import org.scalatestplus.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
class PlaylistReaderSpec extends AnyFlatSpec {
  behavior of "read"

  it should "read test playlist" in {
    // Prepare
    val playlistPath = Paths.get(ClassLoader.getSystemResource("playlist/playlist.json").toURI)

    // Execute
    val playlist = PlaylistReader.read(playlistPath)

    // Assert
    val rootDir = playlistPath.getParent
    assert(playlist.songs.size == 2)
    val song1 = playlist.songs(0)
    val song1Dir = rootDir.resolve("song1")
    assert(song1.title == "Song 1")
    assert(song1.tracks.size == 5)
    assert(song1.tracks(0).path == song1Dir.resolve("main.wav"))
    assert(song1.tracks(0).trackType == MainTrack)
    assert(song1.tracks(1).path == song1Dir.resolve("track1.wav"))
    assert(song1.tracks(1).trackType == BassTrack)
    assert(song1.tracks(2).path == song1Dir.resolve("track2.wav"))
    assert(song1.tracks(2).trackType == BeatTrack)
    assert(song1.tracks(2).muted)
    assert(song1.tracks(3).path == song1Dir.resolve("track3.wav"))
    assert(song1.tracks(3).trackType == SynthTrack)
    assert(song1.tracks(3).muted)
    assert(song1.tracks(4).path == song1Dir.resolve("track4.wav"))
    assert(song1.tracks(4).trackType == SynthTrack)
    assert(song1.tracks(4).muted)
    assert(song1.loops.size == 2)
    assert(song1.loops(0).start == 143002)
    assert(song1.loops(0).end == 156321)
    assert(song1.loops(1).start == 216540)
    assert(song1.loops(1).end == 217601)
    val song2 = playlist.songs(1)
    val song2Dir = rootDir.resolve("song2")
    assert(song2.title == "Song 2")
    assert(song2.tracks.size == 1)
    assert(song2.tracks(0).path == song2Dir.resolve("main.wav"))
    assert(song2.tracks(0).trackType == MainTrack)
    assert(song2.loops.isEmpty)
  }
}
