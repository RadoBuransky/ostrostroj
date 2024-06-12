#pragma once

#include "midi_note.hpp"

class CommandSeq {
    private:
        const std::vector<MidiNote> seq;
        const int context;
        const uint8_t channel;
        size_t seq_counter;
        std::chrono::steady_clock::time_point seq_started;
    public:
        CommandSeq(std::vector<MidiNote> _seq, int _context, uint8_t _channel);
        bool note(uint8_t channel, uint8_t note, bool on, unsigned int clock, bool running);
        int get_context() const;
};