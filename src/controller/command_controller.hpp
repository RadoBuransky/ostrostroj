#pragma once

#include "command_seq.hpp"

enum Command {
    NOOP = 0,
    RESTART = 1,
    SHUTDOWN = 2
};

class CommandController {
    private:
        std::array<CommandSeq, 2> commands;
    public:
        CommandController();
        virtual ~CommandController() = default;
        Command note(uint8_t channel, uint8_t note, bool on, unsigned int clock, bool running);
};