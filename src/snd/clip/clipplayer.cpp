#include "common.hpp"
#include "clipplayer.hpp"

// 10ms @ 96kHz
static constexpr snd_pcm_uframes_t FADE_FRAMES = 10 * 96;

void ClipPlayer::update_pointers(ClipBlock& _block) {
    current_frame = _block.get_buffer().data();
    end_frame = current_frame + (_block.get_buffer_frames() * _block.get_channels());
}

ClipPlayer::ClipPlayer(Clip& _clip, bool _loop, snd_pcm_uframes_t _latency_frames, bool predelay):
    clip(_clip),
    loop(_loop),
    latency_samples(_latency_frames * _clip.get_head().get_channels()),
    fade_samples(FADE_FRAMES * _clip.get_head().get_channels()),
    fade((_loop ? fade_samples : 0) + (predelay ? latency_samples : 0)),
    block(_clip.get_head()),
    current_frame(nullptr),
    end_frame(nullptr),
    position(0) {
    update_pointers(block.get());
}

void ClipPlayer::drain() {
    fade = -fade_samples - latency_samples;
}