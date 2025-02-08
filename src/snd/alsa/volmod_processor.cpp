#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "volmod_processor.hpp"

static const std::vector<snd_seq_event_t> EMPTY_RESULT = {};

VolModProcessor::VolModProcessor() {    
}

std::vector<snd_seq_event_t> VolModProcessor::process(snd_seq_event_t &event) {
    return EMPTY_RESULT;
}