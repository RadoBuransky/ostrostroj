/*
#include "common.h"
#include "track.hpp"

bool Track::push_next_frame() {
    if (next_frame.empty()) {
        return true;
    }
    for (unsigned int channel = 0; channel < channels.size(); channel++) {
        if (!channels[channel].get().push(std::move(next_frame.at(channel)))) {
            if (channel != 0) {
                SPDLOG_WARN("Next frame overflow! [track={}, {} ch]", track_number, channel);
                next_frame.clear();
            }
            return false;
        }
    }
    next_frame.clear();
    return true;
}

void Track::pop_next_frame(float sample) {    
    next_frame.push_back(sample);
    for (unsigned int channel = 1; channel < channels.size(); channel++) {
        if (!track_node.pop(sample)) {
            next_frame.clear();
            return;
        }
        next_frame.push_back(sample);
    }
}

Track::Track(int _track_number, PcmFifo& channel):
    Track(_track_number, std::vector<std::reference_wrapper<PcmFifo>>({std::ref(channel)})) {
}

Track::Track(int _track_number, PcmFifo& left_channel, PcmFifo& right_channel):
    Track(_track_number, std::vector<std::reference_wrapper<PcmFifo>>({std::ref(left_channel), std::ref(right_channel)})) {
}

Track::Track(int _track_number, std::vector<std::reference_wrapper<PcmFifo>> _channels):
    track_number(_track_number),
    channels(_channels),
    dynamic_node(DynamicNode()),
    track_node(TrackNode(dynamic_node)) {
}

void Track::set_mute(bool mute) {
    track_node.set_mute(mute);
}

void Track::start() {
    track_node.start();
}

void Track::stop() {
    track_node.stop();
}

void Track::set_node(std::unique_ptr<Node>&& node) {
    dynamic_node.set_parent(std::move(node));
}

void Track::reset_node() {
    dynamic_node.reset_parent();
}

void Track::fill_output() {
    if (!push_next_frame()) {
        return;
    }
    const int channels_size = channels.size();
    int channel = channels_size - 1;
    bool overflow = false;
    float sample;
#ifdef PROFILING
    Profiler& profiler = Profiler::get();
#endif
    while (!overflow && track_node.pop(sample)) {
        channel = (channel + 1) % channels_size;
        overflow = !channels[channel].get().push(std::move(sample));
#ifdef PROFILING
        profiler.engine_samples_pushed++;
#endif
    }
    SPDLOG_DEBUG("Track full. [{}]", track_number);
    if (overflow) {
        if (channel != 0) {
            // TODO: It is possible that alsapcm popped channel 0 but not popped channel 1 yet. Concurrency.
            SPDLOG_WARN("FIFO not channel-aligned! [track={}, {}/{} ch]", track_number, channel, channels_size);
            return;
        }
        pop_next_frame(sample);
    } else {
        SPDLOG_WARN("Track underrun!");
    }
}
*/