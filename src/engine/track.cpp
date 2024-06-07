#define SPDLOG_ACTIVE_LEVEL 2

#include "common.hpp"
#include "track.hpp"

bool Track::pop(float& _sample) {
    _sample = 0.0;
    if (clip_players.empty()) {
        return true;
    }
    
    float clip_sample;
    std::vector<std::unique_ptr<ClipPlayer>>::iterator it = clip_players.begin();
    while (it != clip_players.end()) {
        if ((*it)->pop(clip_sample)) {
            _sample += clip_sample;
            it++;
        } else {
            SPDLOG_DEBUG("TRAK{} clip removed", track_number);
            it = clip_players.erase(it);
        }
    }
    _sample = (_sample > 1.0) ? 1.0 : (_sample < -1.0 ? -1.0 :_sample);
    return true;
}

Track::Track(int _track_number, int _channels, snd_pcm_uframes_t _period_size, uint8_t _periods, bool _loop):
    track_number(_track_number),
    channels(_channels),
    period_size(_period_size),
    periods(_periods),
    loop(_loop),
    fifo(std::make_unique<InterleavedFifo>(_period_size * _channels * 2)),
    clip_players(),
    sample(0),
    sample_pending(false) {
}

Track::~Track() {
}

void Track::run() {
    float in_sample;
    try {
        if (sample_pending && !fifo->push(std::move(sample))) {
            SPDLOG_TRACE("TRAK{} FIFO overrun", track_number);
            return;
        }
        do {
            if ((sample_pending = pop(in_sample))) {
                sample = in_sample;
            }
        } while (sample_pending && fifo->push(std::move(sample)));
        if (sample_pending) {
            SPDLOG_TRACE("TRAK{} FIFO filled up", track_number);
        } else {
            SPDLOG_TRACE("TRAK{} done", track_number);
        }
    } catch(std::exception const& e) {
        SPDLOG_ERROR("TRAK{} failed {}", track_number, e.what());
    }
}

InterleavedFifo& Track::get_fifo() const {
    return *fifo.get();
}

int Track::get_track_number() const {
    return track_number;
}

int Track::get_channels() const {
    return channels;
}

/**
 * Not thread safe!
*/
void Track::add_clip(Clip& clip, snd_pcm_uframes_t latency, bool predelay) {
    if (loop) {
        clear(false);
    }
    // Magic number measured experimentally. Needs to be updated whenever we change period, period size, ...
    snd_pcm_uframes_t compensated_latency = std::max(((float)latency - (((float)periods + 2.9) * (float)period_size)), 0.0);
    clip_players.push_back(std::make_unique<ClipPlayer>(clip, loop, compensated_latency, predelay));
    SPDLOG_DEBUG("TRAK{} clip added [compensated_latency={},predelay={}]", track_number, compensated_latency, predelay);
}

/**
 * Not thread safe!
*/
void Track::clear(bool drop) {
    if (drop) {
        PcmSample_s24_3le dropped;
        while (fifo->pop(dropped)) {
            // NOOP
        }
        clip_players.clear();
        return;
    }
    for (auto& clip_player: clip_players) {
        clip_player->drain();
    }
}
