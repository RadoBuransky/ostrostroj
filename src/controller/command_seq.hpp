#pragma once

#include "midi_note.hpp"

class CommandSeq {
    private:
        std::vector<MidiNote> seq;
        ssize_t seq_counter;
        std::chrono::steady_clock::time_point seq_started;
    public:
        CommandSeq(std::vector<MidiNote> _seq, int _context);
        bool note(uint8_t channel, uint8_t note, bool on, unsigned int clock, bool running);
        int get_context();
};