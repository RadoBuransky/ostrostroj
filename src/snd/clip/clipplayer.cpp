#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "clipplayer.hpp"

// 30ms @ 96kHz
static constexpr snd_pcm_uframes_t FADE_FRAMES = 30 * 96;

void ClipPlayer::update_pointers(ClipBlock& _block) {
    current_frame = _block.get_buffer().data();
    end_frame = current_frame + (_block.get_buffer_frames() * _block.get_channels());
}

void ClipPlayer::fade_out() {
    if (fade == 0) { 
        fade = -fade_samples;       
    } else {
        if (fade > 0) {
            if (fade > fade_samples) {
                fade = 0;
            } else {
                // We're fading-in so let's turn back and fade-out
                fade = -fade;
            }
        }
    }
}

void ClipPlayer::fade_in() {
    if (fade == 0) { 
        fade = fade_samples;       
    } else {
        if (fade < 0) {
            // We're fading-out so let's turn back and fade-in
            fade = -fade;
        }
    }
}

ClipPlayer::ClipPlayer(Clip& _clip, bool _loop, snd_pcm_uframes_t _latency_frames, bool predelay):
    clip(_clip),
    loop(_loop),
    latency_samples(_latency_frames * _clip.get_head().get_channels()),
    fade_samples(_loop ? (FADE_FRAMES * _clip.get_head().get_channels()) : 0),
    fade((_loop ? fade_samples : 0) + (predelay ? latency_samples : 0)),
    block(_clip.get_head()),
    current_frame(nullptr),
    end_frame(nullptr),
    position(0),
    draining(false) {
    update_pointers(block.get());
    if (fade_samples > latency_samples) {
        throw OstrostrojException(fmt::format("CLIP  fade is too long! [fade_samples={},latency_samples={}]", fade_samples, latency_samples));
    }
}

void ClipPlayer::drain() {
    draining = true;
    // Magic number because this is called as a reaction to PC MIDI message which comes before actual change
    fade = -(fade_samples / 2) -latency_samples;
}

Clip& ClipPlayer::get_clip() {
    return clip;
}

float ClipPlayer::get_position() {
    return (float) position / (float) clip.get_frames();
}