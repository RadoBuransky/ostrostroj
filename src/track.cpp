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
            it = clip_players.erase(it);
        }
    }
    _sample = (_sample > 1.0) ? 1.0 : _sample;
    return !clip_players.empty();
}

void Track::single_loop_run(ClipPlayer& clip_player, InterleavedFifo& fifo_ref) {
    float in_sample;
    do {
        clip_player.pop(in_sample);
    } while (fifo_ref.push(std::move(sample)));
}

void Track::generic_run(InterleavedFifo& fifo_ref) {
    float in_sample;
    do {
        if ((sample_pending = pop(in_sample))) {
            sample = in_sample;
        } else {
            if (loop) {
                throw OstrostrojException(fmt::format("TRAK{} underrun!", track_number));
            }
            // Be careful because we're desyncing tracks here
            SPDLOG_TRACE("TRAK{} no_xrun silence", track_number);
            sample.silence();
        }
    } while (fifo_ref.push(std::move(sample)));
}

Track::Track(int _track_number, int _channels, snd_pcm_uframes_t _period_size, bool _loop):
    track_number(_track_number),
    channels(_channels),
    loop(_loop),
    fifo(std::make_unique<InterleavedFifo>(_period_size * 32 * _channels)), // TODO: Why 32?
    sample(0),
    sample_pending(false) {
}

Track::~Track() {
}

void Track::run() {
    try {
        if (sample_pending && !fifo->push(std::move(sample))) {
            SPDLOG_TRACE("TRAK{} FIFO overrun", track_number);
            return;
        }
        if (loop && clip_players.size() == 1) {
            single_loop_run(*clip_players.at(0), *fifo);
        } else {
            generic_run(*fifo);
        }
        SPDLOG_TRACE("TRAK{} FIFO filled up", track_number);
    } catch(std::exception const& e) {
        SPDLOG_ERROR("TRAK{} failed. {}", track_number, e.what());
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