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

  it should "not fail during construction" in {
    AudioPlayer(playlist, OstrostrojConfig.audio).close()
  }

  it should "play a song and then stop" in {
    withAudioPlayer(playlist) { audioPlayer =>
      audioPlayer.play()
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

  @tailrec
  private def waitUntilPlaybackIsDone(audioPlayer: AudioPlayer): Unit = {
    Thread.sleep(100)
    val status = audioPlayer.status
    if (!status.done) {
      waitUntilPlaybackIsDone(audioPlayer)
    }
  }
}
