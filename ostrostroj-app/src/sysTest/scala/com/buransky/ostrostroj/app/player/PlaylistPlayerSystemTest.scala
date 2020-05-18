package com.buransky.ostrostroj.app.player

import akka.actor.testkit.typed.scaladsl.BehaviorTestKit
import akka.actor.typed.ActorRef
import com.buransky.ostrostroj.app.common.OstrostrojConfig
import com.buransky.ostrostroj.app.show.{Playlist, PlaylistReader}
import com.buransky.ostrostroj.app.sysTest.BaseSystemTest
import javax.sound.sampled.AudioFormat.Encoding
import org.junit.runner.RunWith
import org.scalatest.BeforeAndAfterAll
import org.scalatestplus.junit.JUnitRunner
import scala.concurrent.duration._

@RunWith(classOf[JUnitRunner])
class PlaylistPlayerSystemTest extends BaseSystemTest with PlaylistPlayerFixture {
  behavior of "constructor"

  ignore should "initialize audio output device as expected" in {
    val behaviorTestKit = BehaviorTestKit(PlaylistPlayer(playlist))
    val playlistPlayer: PlaylistPlayerBehavior = behaviorTestKit.currentBehavior match {
      case ppb: PlaylistPlayerBehavior => ppb
      case _ => fail()
    }

    assert(playlistPlayer.sourceDataLine.getFormat.getSampleRate == 44100)
    assert(playlistPlayer.sourceDataLine.getFormat.getChannels == 2)
    assert(playlistPlayer.sourceDataLine.getFormat.getSampleSizeInBits == 16)
    assert(!playlistPlayer.sourceDataLine.getFormat.isBigEndian)
    assert(playlistPlayer.sourceDataLine.getFormat.getFrameSize == 4)
    assert(playlistPlayer.sourceDataLine.getFormat.getEncoding == Encoding.PCM_SIGNED)
    assert(playlistPlayer.gainControl.getValue == 0.0)

    val status = playlistPlayer.currentPlayerStatus()
    assert(!status.isPlaying)
    assert(status.loop.isEmpty)
    assert(status.masterGainDb == 0.0)
    assert(status.playlist == playlist)
    assert(status.songIndex == 0)
    assert(status.songPosition.toMillis == 0)
    assert(status.songDuration.toMillis == 8000)
  }

  it should "start playback" in {
    // Execute
    playlistPlayerRef ! PlaylistPlayer.Play

    // Wait until playback starts
    val testProbe = testKit.createTestProbe[AnyRef]()
    do {
      playlistPlayerRef ! PlaylistPlayer.GetStatus(testProbe.ref)
      Thread.sleep(100)
    } while (!testProbe.expectMessageType[PlaylistPlayer.PlayerStatus](1.second).isPlaying)

    // Wait until playback is done
    do {
      playlistPlayerRef ! PlaylistPlayer.GetStatus(testProbe.ref)
      Thread.sleep(100)
    } while (testProbe.expectMessageType[PlaylistPlayer.PlayerStatus](1.second).isPlaying)

  }
}

trait PlaylistPlayerFixture extends BeforeAndAfterAll { this: BaseSystemTest =>
  val playlist: Playlist = PlaylistReader.read(OstrostrojConfig.playlistPath)
  val playlistPlayerRef: ActorRef[PlaylistPlayer.Command] = testKit.spawn(PlaylistPlayer(playlist), "player")

  override def beforeAll(): Unit = {
  }

  override def afterAll(): Unit = {
  }
}