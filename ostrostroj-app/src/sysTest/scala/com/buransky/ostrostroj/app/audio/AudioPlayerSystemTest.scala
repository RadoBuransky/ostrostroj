package com.buransky.ostrostroj.app.audio

import com.buransky.ostrostroj.app.common.OstrostrojConfig
import com.buransky.ostrostroj.app.show.{Playlist, PlaylistReader}
import com.buransky.ostrostroj.app.sysTest.BaseSystemTest
import org.scalatest.SequentialNestedSuiteExecution
import org.scalatest.flatspec.AnyFlatSpecLike

import scala.annotation.tailrec

class AudioPlayerSystemTest extends AnyFlatSpecLike with SequentialNestedSuiteExecution {
  BaseSystemTest

  private val playlist = PlaylistReader.read(OstrostrojConfig.playlistPath)

  behavior of "audio player"

  it should "not fail during construction" in {
    withAudioPlayer(playlist)(_.close())
  }

  it should "play a single song and then stop" in {
    withAudioPlayer(playlist.subplaylist(1)) { audioPlayer =>
      audioPlayer.play()
      waitUntilPlaybackIsDone(audioPlayer)
    }
  }

  it should "be able to automatically switch to the next song" in {
    withAudioPlayer(playlist) { audioPlayer =>
      audioPlayer.play()
      waitUntilPlaybackIsDone(audioPlayer)
    }
  }

  it should "loop a couple of times" in {
    withAudioPlayer(playlist.copy(songs = List(playlist.songs.head))) { audioPlayer =>
      audioPlayer.play()
      val loop = playlist.songs.head.loops.head
      waitUntil(audioPlayer)(_.position.value >= loop.start)
      audioPlayer.toggleLooping()
      var loopCount = 0
      var prevPosition = audioPlayer.status.position
      waitUntil(audioPlayer) { status =>
        if (status.position.value < prevPosition.value) {
          loopCount += 1
        }
        prevPosition = status.position
        loopCount >= 3
      }
      audioPlayer.toggleLooping()
      waitUntilPlaybackIsDone(audioPlayer)
    }
  }

  it should "loop normal, softer and then harder" in {
    withAudioPlayer(playlist.copy(songs = List(playlist.songs.head))) { audioPlayer =>
      audioPlayer.play()
      val loop = playlist.songs.head.loops.head
      waitUntil(audioPlayer)(_.position.value >= loop.start)
      audioPlayer.toggleLooping()
      var loopCount = 0
      var prevPosition = audioPlayer.status.position
      waitUntil(audioPlayer) { status =>
        if (status.position.value < prevPosition.value) {
          loopCount match {
            case 0 => audioPlayer.softer()
            case 1 =>
              audioPlayer.harder()
              audioPlayer.harder()
            case _ =>
          }
          loopCount += 1
        }
        prevPosition = status.position
        loopCount >= 2
      }
      audioPlayer.toggleLooping()
      waitUntilPlaybackIsDone(audioPlayer)
    }
  }

  it should "be able to gradually decrease and increase volume" in {
    withAudioPlayer(playlist.subplaylist(1)) { audioPlayer =>
      audioPlayer.play()
      var i = 0
      waitUntil(audioPlayer) { status =>
        i match {
          case 0 => audioPlayer.volumeDown()
          case 1 => audioPlayer.volumeUp()
        }
        if (status.volume == 0) {
          i = 1
        }
        status.volume >= 0.99
      }
      waitUntilPlaybackIsDone(audioPlayer)
    }
  }

  private def withAudioPlayer(playlist: Playlist)(f: (AudioPlayer) => Any): Unit = {
    val audioPlayer = AudioPlayer(playlist, OstrostrojConfig.audio)
    try {
      f(audioPlayer)
    } finally {
      audioPlayer.close()
    }
  }

  private def waitUntilPlaybackIsDone(audioPlayer: AudioPlayer): Unit = waitUntil(audioPlayer)(_.done)

  @tailrec
  private def waitUntil(audioPlayer: AudioPlayer)(p: (AudioPlayerStatus) => Boolean): Unit = {
    Thread.sleep(100)
    val status = audioPlayer.status
    if (!p(status)) {
      waitUntil(audioPlayer)(p)
    }
  }
}
