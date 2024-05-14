#define SPDLOG_ACTIVE_LEVEL 2

#include "common.hpp"
#include "track.hpp"

bool Track::pop(float& _sample) {
    float clip_sample;
    _sample = 0.0;
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
    return !clip_players.empty();
}

Track::Track(int _track_number, int _channels, snd_pcm_uframes_t _period_size, bool _loop):
    track_number(_track_number),
    channels(_channels),
    loop(_loop),
    fifo(std::make_unique<InterleavedFifo>(_period_size * 32 * _channels)), // TODO: Why 32?
    clip_players(4), // Preallocate 4 slots
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
            SPDLOG_DEBUG("TRAK{} done", track_number);
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

void Track::add_clip(Clip& clip) {
    if (loop) {
        clear(false);
    }
    clip_players.push_back(std::make_unique<ClipPlayer>(clip, loop));
    SPDLOG_DEBUG("TRAK{} clip added", track_number);
}

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