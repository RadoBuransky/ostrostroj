#include "common.hpp"

#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>

#include "track.hpp"

bool Track::pop(float& _sample) {
    _sample = 0.0;
    if (clip_players.empty()) {
        return true;
    }
    if (warp.pop(_sample)) {
        return true;
    }    
    do {
        float clip_sample;
        _sample = 0.0;
        for (auto& clip_player: clip_players) {
            if (clip_player->pop(clip_sample)) {
                _sample += saturation.saturate(clip_sample);
            }
        }
        // Brickwall limitter
        _sample = (_sample > 1.0) ? 1.0 : (_sample < -1.0 ? -1.0 :_sample);
    } while (!warp.pushnpop(_sample));
    return true;
}

Track::Track(int _track_number, int _channels, snd_pcm_uframes_t _period_size, uint8_t _periods):
    track_number(_track_number),
    channels(_channels),
    period_size(_period_size),
    periods(_periods),
    fifo(std::make_unique<InterleavedFifo>(_period_size * _channels * 2)),
    clip_players(),
    sample(0),
    sample_pending(false),
    warp(_channels, _track_number),
    saturation(_track_number) {
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
ClipPlayer& Track::add_clip(Clip& clip) {
    std::unique_ptr<ClipPlayer>& clip_player = clip_players.emplace_back(std::make_unique<ClipPlayer>(clip));
    bool warp_enabled = clip.is_warp_enabled();
    warp.set_bypass(!warp_enabled);
    SPDLOG_DEBUG("TRAK{} clip added [warp_enabled={},addr=0x{:x}]", track_number, warp_enabled, (long) clip_player.get());
    return *clip_player;
}

/**
 * Not thread safe!
*/
void Track::remove_clip_player(Clip& clip) {
    std::vector<std::unique_ptr<ClipPlayer>>::iterator it = clip_players.begin();
    while (it != clip_players.end()) {
        if ((*it)->get_clip().get_path().compare(clip.get_path()) == 0) {
            ClipPlayer* to_remove = (*it).get();
            it = clip_players.erase(it);
            SPDLOG_DEBUG("TRAK{} clip player removed [clip={},addr=0x{:x}]", track_number, clip.get_path().c_str(), (long) to_remove);
            return;
        } else {
            it++;
        }
    }
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
}

float Track::get_position(std::filesystem::path& clip_path) {
    if (clip_players.size() == 1) {
        return clip_players.at(0)->get_position();
    }
    for (auto& clip_player: clip_players) {
        if (clip_player->get_clip().get_path().compare(clip_path) == 0) {
            return clip_player->get_position();
        }
    }
    return 0.0;
}

void Track::set_saturation(float _saturation) {
    saturation.set_drive(_saturation);
    saturation.set_dry_wet(std::min(1.0f, _saturation * 1.5f)); // Reaches 100% wet before 100% saturation
}

float Track::get_saturation() {
    return saturation.get_dry_wet();
}