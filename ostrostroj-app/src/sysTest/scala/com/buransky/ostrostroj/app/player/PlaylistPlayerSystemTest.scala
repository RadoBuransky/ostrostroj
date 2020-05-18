package com.buransky.ostrostroj.app.player

import akka.actor.testkit.typed.scaladsl.{BehaviorTestKit, TestProbe}
import akka.actor.typed.ActorRef
import com.buransky.ostrostroj.app.common.OstrostrojConfig
import com.buransky.ostrostroj.app.player.PlaylistPlayer.PlayerStatus
import com.buransky.ostrostroj.app.show.{Playlist, PlaylistReader}
import com.buransky.ostrostroj.app.sysTest.BaseSystemTest
import javax.sound.sampled.AudioFormat.Encoding
import org.junit.runner.RunWith
import org.scalatest.BeforeAndAfterAll
import org.scalatestplus.junit.JUnitRunner

import scala.annotation.tailrec
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

  it should "play the whole song 1" in {
    val testProbe = testKit.createTestProbe[AnyRef]()
    assert(getStatus(testProbe).songPosition.toMillis == 0, "Position should be at the beginning")

    // Start playback
    startPlayback(testProbe)

    // Wait until playback is done
    var oldPos: Long = 0
    whileStatus(testProbe, _.isPlaying) { status =>
      assert(status.songPosition.toMillis >= oldPos, "Current position should be always growing.")
      oldPos = status.songPosition.toMillis
    }

    // Get status
    val status = getStatus(testProbe)
    assert(!status.isPlaying)
    assert(status.songIndex == 0)
    assert(status.songPosition == status.songDuration, "Position should be at the end")
  }

  private def startPlayback(testProbe: TestProbe[AnyRef]): Unit = {
    playlistPlayerRef ! PlaylistPlayer.Play
    whileStatus(testProbe, !_.isPlaying)(_ => ())
  }

  @tailrec
  private def whileStatus(testProbe: TestProbe[AnyRef], p: (PlayerStatus) => Boolean)(f: (PlayerStatus) => Any): Unit = {
    val status = getStatus(testProbe)
    if (p(status)) {
      f(status)
      Thread.sleep(100)
      whileStatus(testProbe, p)(f)
    }
  }

  private def getStatus(testProbe: TestProbe[AnyRef]): PlayerStatus = {
    playlistPlayerRef ! PlaylistPlayer.GetStatus(testProbe.ref)
    testProbe.expectMessageType[PlaylistPlayer.PlayerStatus](1.second)
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