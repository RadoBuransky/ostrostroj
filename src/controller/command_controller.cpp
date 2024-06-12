#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "command_controller.hpp"

CommandController::CommandController() {    
}

Command CommandController::note(uint8_t channel, uint8_t note, bool on, unsigned int clock, bool running) {
    return NOOP;
}