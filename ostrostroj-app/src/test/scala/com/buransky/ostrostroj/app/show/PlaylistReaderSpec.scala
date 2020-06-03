package com.buransky.ostrostroj.app.show

import java.nio.file.{Path, Paths}

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
    assertSong1(playlist.songs(0), rootDir)
    assertSong2(playlist.songs(1), rootDir)
  }

  private def assertSong1(song1: Song, rootDir: Path) = {
    val song1Dir = rootDir.resolve("song1")
    assert(song1.title == "Song 1")
    assert(song1.path == song1Dir.resolve("main.wav"))
    assert(song1.loops.size == 1)
    val firstLoop = song1.loops(0)
    assert(firstLoop.start == 143002)
    assert(firstLoop.endExclusive == 156321)
    assert(firstLoop.tracks.size == 5)
    assert(firstLoop.tracks(0).rangeMin == -1)
    assert(firstLoop.tracks(0).rangeMax == 1)
    assert(firstLoop.tracks(0).fade == 1)
    assert(firstLoop.tracks(0).path == song1Dir.resolve("loop1s2.wav"))
    assert(firstLoop.tracks(1).rangeMin == -2)
    assert(firstLoop.tracks(1).rangeMax == -2)
    assert(firstLoop.tracks(1).fade == 0)
    assert(firstLoop.tracks(1).path == song1Dir.resolve("loop1s1.wav"))
    assert(firstLoop.tracks(2).rangeMin == 0)
    assert(firstLoop.tracks(2).rangeMax == 0)
    assert(firstLoop.tracks(2).fade == 0)
    assert(firstLoop.tracks(2).path == song1Dir.resolve("loop1.wav"))
    assert(firstLoop.tracks(3).rangeMin == 1)
    assert(firstLoop.tracks(3).rangeMax == 1)
    assert(firstLoop.tracks(3).fade == 0)
    assert(firstLoop.tracks(3).path == song1Dir.resolve("loop1h1.wav"))
    assert(firstLoop.tracks(4).rangeMin == 2)
    assert(firstLoop.tracks(4).rangeMax == 2)
    assert(firstLoop.tracks(4).fade == 0)
    assert(firstLoop.tracks(4).path == song1Dir.resolve("loop1h2.wav"))
  }

  private def assertSong2(song2: Song, rootDir: Path) = {
    val song2Dir = rootDir.resolve("song2")
    assert(song2.title == "Song 2")
    assert(song2.path == song2Dir.resolve("main.wav"))
    assert(song2.loops.isEmpty)
  }
}
