#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "clipplayer.hpp"

void ClipPlayer::update_pointers(ClipBlock& _block) {
    current_sample = _block.get_buffer().data();
    end_sample = current_sample + (_block.get_buffer_frames() * _block.get_channels());
}

ClipPlayer::ClipPlayer(Clip& _clip):
    clip(_clip),
    block(_clip.get_head()),
    current_sample(nullptr),
    end_sample(nullptr),
    position(0),
    paused(false),
    gain(1.0f),
    skip_samples(0) {
    update_pointers(block.get());
    SPDLOG_TRACE("CLPPL constructed[clip={},addr=0x{:x}]", clip.get_path().c_str(), (long) this);
}

ClipPlayer::~ClipPlayer() {
    SPDLOG_TRACE("CLPPL destructed[clip={},addr=0x{:x}]", clip.get_path().c_str(), (long) this);
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

void ClipPlayer::skip(std::chrono::steady_clock::duration period) {
    int64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(period).count();
    skip_samples = (ms * clip.get_info().samplerate * clip.get_info().channels) / 1000;
}