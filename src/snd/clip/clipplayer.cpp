#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "clipplayer.hpp"

// 30ms @ 96kHz
static constexpr snd_pcm_uframes_t FADE_FRAMES = 30 * 96;

void ClipPlayer::update_pointers(ClipBlock& _block) {
    current_sample = _block.get_buffer().data();
    end_sample = current_sample + (_block.get_buffer_frames() * _block.get_channels());
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

ClipPlayer::ClipPlayer(Clip& _clip):
    clip(_clip),
    fade_samples(FADE_FRAMES * _clip.get_head().get_channels()),
    fade(fade_samples),
    block(_clip.get_head()),
    current_sample(nullptr),
    end_sample(nullptr),
    position(0),
    draining(false),
    paused(false),
    gain(1.0f) {
    update_pointers(block.get());
    SPDLOG_TRACE("CLPPL constructed[clip={},addr=0x{:x}]", clip.get_path().c_str(), (long) this);
}

ClipPlayer::~ClipPlayer() {
    SPDLOG_TRACE("CLPPL destructed[clip={},addr=0x{:x}]", clip.get_path().c_str(), (long) this);
}

void ClipPlayer::drain() {
    draining = true;
    fade = -(fade_samples / 2);
}

Clip& ClipPlayer::get_clip() {
    return clip;
}

float ClipPlayer::get_position() {
    return std::min(1.0f, (float) position / (float) (clip.get_frames() * clip.get_info().channels));
}

void ClipPlayer::set_paused(bool _paused) {    
    paused = _paused;
    SPDLOG_DEBUG("CLPPL paused[{}]", paused);
}

bool ClipPlayer::is_paused() {
    return paused;
}

void ClipPlayer::set_gain(float _gain) {
    gain = std::max(std::min(_gain, 1.0f), 0.0f);
    SPDLOG_DEBUG("CLPPL gain[{}]", gain);
}

float ClipPlayer::get_gain() {
    return gain;
}