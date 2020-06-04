package com.buransky.ostrostroj.app.show

import java.io.FileInputStream
import java.nio.file.Path

import play.api.libs.json.{JsValue, Json}

case class Playlist(songs: Seq[Song]) {
  def subplaylist(fromSong: Int): Playlist = this.copy(songs = songs.slice(fromSong, songs.length - 1))
}
case class Song(title: String, path: Path, loops: Seq[Loop])
case class Loop(start: Int, endExclusive: Int, tracks: Seq[Track])
case class Track(rangeMin: Int, rangeMax: Int, fade: Int, path: Path) {
  def channelLevel(level: Int): Double = {
    if ((fade == 0) || (level >= rangeMin && level <= rangeMax))
      1.0
    else {
      if (level < rangeMin)
        1.0 - ((rangeMin - level) / fade)
      else
        1.0 - ((level - rangeMax) / fade)
    }
  }
}

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
    processJsonFile(songDir.resolve(songDir.getFileName.toString + ".json")) { songJson =>
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
        val range = (track \ "range").as[Seq[Int]]
        Track(
          rangeMin = range.min,
          rangeMax = range.max,
          fade = (track \ "fade").asOpt[Int].getOrElse(0),
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