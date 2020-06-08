package com.buransky.ostrostroj.app.audio

import com.buransky.ostrostroj.app.common.OstrostrojConfig
import com.buransky.ostrostroj.app.show.PlaylistReader
import com.buransky.ostrostroj.app.sysTest.BaseSystemTest
import org.junit.runner.RunWith
import org.scalatestplus.junit.JUnitRunner

import scala.annotation.tailrec

@RunWith(classOf[JUnitRunner])
class AudioPlayerSystemTest extends BaseSystemTest {
  private val playlist = PlaylistReader.read(OstrostrojConfig.playlistPath)

  behavior of "audio player"

  it should "not fail during construction" in {
    AudioPlayer(playlist, OstrostrojConfig.audio).close()
  }

  it should "play a song and then stop" in {
    val audioPlayer = AudioPlayer(playlist, OstrostrojConfig.audio)
    audioPlayer.play()
    waitUntilPlaybackIsDone(audioPlayer)
    audioPlayer.close()
  }

  @tailrec
  private def waitUntilPlaybackIsDone(audioPlayer: AudioPlayer): Unit = {
    Thread.sleep(100)
    val status = audioPlayer.status
    if (!status.isPaused) {
      waitUntilPlaybackIsDone(audioPlayer)
    }
  }
}
