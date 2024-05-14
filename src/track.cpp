#define SPDLOG_ACTIVE_LEVEL 2

#include "common.hpp"
#include "track.hpp"

/*
Track::Track(int _track_number, int _channels, snd_pcm_uframes_t _period_size, bool _no_xrun):
    track_number(_track_number),
    channels(_channels),
    no_xrun(_no_xrun),
    fifo(std::make_unique<InterleavedFifo>(_period_size * 32 * _channels)), // TODO: 32?
    dynamic_node(),
    track_node(dynamic_node),
    sample(0),
    sample_pending(false) {
}
*/

bool Track::pop(float& sample) {
    // TODO:
    return false;
}

Track::Track(int _track_number, int _channels, snd_pcm_uframes_t _period_size, bool _loop):
    track_number(_track_number),
    channels(_channels),
    fifo(std::make_unique<InterleavedFifo>(_period_size * 32 * _channels)), // TODO: Why 32?
    loop(_loop),
    sample(0),
    sample_pending(false) {
}

Track::~Track() {
}

void Track::run() {
    InterleavedFifo& fifo_ref = *fifo.get();
    float in_sample;
    snd_pcm_uframes_t frames_pushed = 0;
    try {
        if (sample_pending && !fifo_ref.push(std::move(sample))) {
            SPDLOG_TRACE("TRAK{} FIFO overrun", track_number);
            return;
        }
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
            frames_pushed++;
        } while (fifo_ref.push(std::move(sample)));
        SPDLOG_TRACE("TRAK{} FIFO filled up [frames_pushed={}]", track_number, frames_pushed - 1);
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
    // TODO:
}

void Track::clear(bool drop) {
    // TODO:    
}

/*
void Track::reset_node() {
    dynamic_node.reset_parent();
}

void Track::set_node(std::unique_ptr<Node>&& node) {
    dynamic_node.set_parent(std::move(node));
}

void Track::drop() {
    PcmSample_s24_3le dropped;
    while (fifo->pop(dropped)) {
        // NOOP
    }
}
*/