package com.buransky.ostrostroj.app.show.playlist

import java.io.FileInputStream
import java.nio.file.Path

import com.buransky.ostrostroj.app.common.OstrostrojException
import play.api.libs.json.{JsValue, Json}

final case class Playlist(songs: Seq[Song])
final case class Song(title: String, audio: Path, tracks: Seq[Track], loops: Seq[Loop])
final case class Track(audio: Path, trackType: TrackType)
final case class Loop(start: Int, end: Int)

sealed trait TrackType
case object BassTrack extends TrackType
case object BeatTrack extends TrackType
case object SynthTrack extends TrackType

/**
 * https://github.com/playframework/play-json
 */
class PlaylistReader {
  import PlaylistReader._

  def read(rootDir: Path): Playlist = {
    processJsonFile(rootDir.resolve(playlistFileName)) { playlistJson =>
      val songDirs = (playlistJson \ "songs").as[Seq[String]]
      val songs = songDirs.map(songDir => readSong(rootDir.resolve(songDir)))
      Playlist(songs)
    }
  }

  private def readSong(songDir: Path): Song = {
    processJsonFile(songDir.resolve(songFileName)) { songJson =>
      Song(
        title = (songJson \ "title").as[String],
        audio = songDir.resolve((songJson \ "audio").as[String]),
        tracks = readTracks(songDir, songJson),
        loops = readLoops(songJson)
      )
    }
  }

  private def readTracks(songDir: Path, songJson: JsValue): Seq[Track] = {
    (songJson \ "tracks").asOpt[Seq[JsValue]].map { optTracks =>
      optTracks.map { track =>
        Track(
          audio = songDir.resolve((track \ "audio").as[String]),
          trackType = readTrackType((track \ "type").as[String])
        )
      }
    }.getOrElse(Nil)
  }

  private def readTrackType(trackTypeJson: String): TrackType = {
    trackTypeJson match {
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

object PlaylistReader {
  private val playlistFileName = "playlist.json"
  private val songFileName = "song.json"
}