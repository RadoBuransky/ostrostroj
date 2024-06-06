#include "common.hpp"
#include "clipplayer.hpp"

// 30ms @ 96kHz
static constexpr snd_pcm_uframes_t FADE_FRAMES = 30 * 96;

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
    if (fade_samples > latency_samples) {
        throw OstrostrojException(fmt::format("CLIP  fade is too long! [fade_samples={},latency_samples={}]", fade_samples, latency_samples));
    }
}

void ClipPlayer::drain() {
    // Magic number so that fade-in and fade-out somewhat overlap
    fade = -(fade_samples / 2) -latency_samples;
}