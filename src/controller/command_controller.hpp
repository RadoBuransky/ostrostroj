#pragma once

#include "command_seq.hpp"

enum Command {
    NOOP = 0,
    RESTART_DEVICE,
    SHUTDOWN_DEVICE,
    RESTART_SERVICE,
    PC_NEXT,
    PC_PREV
};

class CommandController {
    private:
        std::array<CommandSeq, 3> commands;
    public:
        CommandController(uint8_t channel);
        virtual ~CommandController() = default;
        Command note(uint8_t channel, uint8_t note, bool on, unsigned int clock, bool running);
};