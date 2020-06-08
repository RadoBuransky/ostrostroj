package com.buransky.ostrostroj.app.audio

import com.buransky.ostrostroj.app.common.OstrostrojConfig
import com.buransky.ostrostroj.app.show.{Playlist, PlaylistReader}
import com.buransky.ostrostroj.app.sysTest.BaseSystemTest
import org.junit.runner.RunWith
import org.scalatest.ParallelTestExecution
import org.scalatestplus.junit.JUnitRunner

import scala.annotation.tailrec

@RunWith(classOf[JUnitRunner])
class AudioPlayerSystemTest extends BaseSystemTest with ParallelTestExecution {
  private val playlist = PlaylistReader.read(OstrostrojConfig.playlistPath)

  behavior of "audio player"

  ignore should "" in {
    it should "not fail during construction" in {
      AudioPlayer(playlist, OstrostrojConfig.audio).close()
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
