#define SPDLOG_ACTIVE_LEVEL 2

#include "common.hpp"
#include "track.hpp"

Track::Track(int _track_number, int _channels, snd_pcm_uframes_t _period_size, bool _no_xrun):
    track_number(_track_number),
    channels(_channels),
    no_xrun(_no_xrun),
    // TODO: Increase this to ALSA PCM buffer size?
    fifo(std::make_unique<InterleavedFifo>(_period_size * 2 * _channels)),
    dynamic_node(),
    track_node(dynamic_node),
    sample(0.0),
    sample_pending(false) {
}

Track::~Track() {
}

void Track::run() {
    InterleavedFifo& fifo_ref = *fifo.get();
    PcmSample_s24_3le out_sample;
    try {
        if (sample_pending && !fifo_ref.push(std::move(out_sample))) {
            return;
        }
        do {
            if ((sample_pending = track_node.pop(sample))) {
                out_sample = sample;
            } else {
                if (no_xrun) {
                    // Be careful because we're desyncing tracks here
                    out_sample.silence();
                } else {
                    SPDLOG_WARN("Track {} underrun!", track_number);
                }
            }
        } while (fifo_ref.push(std::move(out_sample)));
    } catch(std::exception const& e) {
        SPDLOG_ERROR("Track {} failed. {}", track_number, e.what());
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