package com.buransky.ostrostroj.app.show

import java.io.FileInputStream
import java.nio.file.Path

import com.buransky.ostrostroj.app.common.OstrostrojException
import play.api.libs.json.{JsValue, Json}

final case class Playlist(songs: Seq[Song])
final case class Song(title: String, tracks: Seq[Track], loops: Seq[Loop])
final case class Track(path: Path, trackType: TrackType, muted: Boolean)
final case class Loop(start: Int, end: Int)

sealed trait TrackType
case object MainTrack extends TrackType
case object BassTrack extends TrackType
case object BeatTrack extends TrackType
case object SynthTrack extends TrackType

/**
 * https://github.com/playframework/play-json
 */
object PlaylistReader {
  private val songFileName = "song.json"

  def read(playlistPath: Path): Playlist = {
    processJsonFile(playlistPath) { playlistJson =>
      val rootDir = playlistPath.getParent
      val songDirs = (playlistJson \ "songs").as[Seq[String]]
      val songs = songDirs.map(songDir => readSong(rootDir.resolve(songDir)))
      Playlist(songs)
    }
  }

  private def readSong(songDir: Path): Song = {
    processJsonFile(songDir.resolve(songFileName)) { songJson =>
      Song(
        title = (songJson \ "title").as[String],
        tracks = readTracks(songDir, songJson),
        loops = readLoops(songJson)
      )
    }
  }

  private def readTracks(songDir: Path, songJson: JsValue): Seq[Track] = {
    (songJson \ "tracks").asOpt[Seq[JsValue]].map { optTracks =>
      optTracks.map { track =>
        Track(
          path = songDir.resolve((track \ "path").as[String]),
          trackType = readTrackType((track \ "type").as[String]),
          muted = (track \ "muted").asOpt[Boolean].getOrElse(false)
        )
      }
    }.getOrElse(Nil)
  }

  private def readTrackType(trackTypeJson: String): TrackType = {
    trackTypeJson match {
      case "main" => MainTrack
      case "bass" => BassTrack
      case "beat" => BeatTrack
      case "synth" => SynthTrack
      case _ => throw new OstrostrojException(s"Unknown track type! [$trackTypeJson]")
    }
  }

  private def readLoops(songJson: JsValue): Seq[Loop] = {
    (songJson \ "loops").asOpt[Seq[JsValue]].map { optLoops =>
      optLoops.map { loop =>
        Loop(
          start = (loop \ "start").as[Int],
          end = (loop \ "end").as[Int]
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