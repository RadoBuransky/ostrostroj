#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "alsamidi_processor.hpp"

AlsaMidiProcessor::AlsaMidiProcessor():
    volMod() {    
}

std::vector<snd_seq_event_t> AlsaMidiProcessor::process(snd_seq_event_t &event) {
    return volMod.process(event);
}

VolModProcessor& AlsaMidiProcessor::get_volmod_processor() {
    return volMod;
}