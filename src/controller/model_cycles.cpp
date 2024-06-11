#include "common.hpp"

#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>

#include "model_cycles.hpp"

static constexpr uint8_t T1_CHANNEL = 0;

ModelCycles::ModelCycles() {
}

snd_seq_event_t ModelCycles::mute_track(size_t track_number, bool muted) {
    snd_seq_event_t result;
    result.type = SND_SEQ_EVENT_CONTROLLER;
    result.data.control.channel = T1_CHANNEL + track_number - 1;
    result.data.control.param = 94; // M:C User Manual OS 1.13, APPENDIX A: MIDI SPECIFICATIONS
    result.data.control.value = muted ? 1 : 0;
    SPDLOG_DEBUG("M:C   mute_track[track_number={},muted={}]", track_number, muted);
    return result;
}