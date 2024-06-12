#pragma once

enum Command {
    NOOP = 0,
    RESTART = 1,
    SHUTDOWN = 2
};

class CommandController {
    public:
        CommandController();
        virtual ~CommandController() = default;
        Command note(uint8_t channel, uint8_t note, bool on, unsigned int clock, bool running);
};