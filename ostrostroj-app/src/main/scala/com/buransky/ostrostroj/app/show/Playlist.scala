package com.buransky.ostrostroj.app.show

import java.io.FileInputStream
import java.nio.file.Path

import play.api.libs.json.{JsValue, Json}

final case class Playlist(songs: Seq[Song])
final case class Song(title: String, path: Path, loops: Seq[Loop])
final case class Loop(start: Int, endExclusive: Int, tracks: Seq[Track])
final case class Track(rangeMin: Int, rangeMax: Int, fade: Int, path: Path)

/**
 * https://github.com/playframework/play-json
 */
object PlaylistReader {
  def read(playlistPath: Path): Playlist = {
    processJsonFile(playlistPath) { playlistJson =>
      val rootDir = playlistPath.getParent
      val songDirs = (playlistJson \ "songs").as[Seq[String]]
      val songs = songDirs.map(songDir => readSong(rootDir.resolve(songDir)))
      Playlist(songs)
    }
  }

  private def readSong(songDir: Path): Song = {
    processJsonFile(songDir.resolve(songDir.getFileName + ".json")) { songJson =>
      Song(
        title = (songJson \ "title").as[String],
        path = songDir.resolve((songJson \ "path").as[String]),
        loops = readLoops(songDir, songJson)
      )
    }
  }

  private def readLoops(songDir: Path, songJson: JsValue): Seq[Loop] = {
    (songJson \ "loops").asOpt[Seq[JsValue]].map { optLoops =>
      optLoops.map { loop =>
        Loop(
          start = (loop \ "start").as[Int],
          endExclusive = (loop \ "end").as[Int],
          tracks = readTracks(songDir, loop)
        )
      }
    }.getOrElse(Nil)
  }

  private def readTracks(songDir: Path, loopJson: JsValue): Seq[Track] = {
    (loopJson \ "tracks").asOpt[Seq[JsValue]].map { optTracks =>
      optTracks.map { track =>
        Track(
          rangeMin = ???,
          rangeMax = ???,
          fade = ???,
          path = songDir.resolve((track \ "path").as[String])
        )
      }
    }.getOrElse(Nil)
  }

  private def processJsonFile[T](file: Path)(action: JsValue => T): T = {
    val inputStream = new FileInputStream(file.toFile);
    try {
      val parsedJson = Json.parse(inputStream)
      action(parsedJson)
    }
    finally {
      inputStream.close()
    }
  }
}