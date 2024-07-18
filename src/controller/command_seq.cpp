#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "command_seq.hpp"

static constexpr std::chrono::steady_clock::duration SEQ_TIMEOUT = std::chrono::seconds(3);

CommandSeq::CommandSeq(std::vector<MidiNote> _seq, int _context, uint8_t _channel):
    seq(_seq),
    context(_context),
    channel(_channel),
    seq_counter(0),
    seq_started(std::chrono::steady_clock::time_point::min()) {
}

bool CommandSeq::note(uint8_t _channel, uint8_t note, bool on, unsigned int, bool running) {
    if (_channel != channel || running) {
        return false;
    }
    if (seq_counter == seq.size()) {
        if (note == seq.back().get_value()) {
            if (on) {
                // Syntakt sends the same note twice
                return false;
            }
            if (std::chrono::steady_clock::now() > seq_started + SEQ_TIMEOUT) {
                seq_counter = 0;
                SPDLOG_DEBUG("CMDSQ command detected[context={}]", context);
                return true;
            }
        }
    } else {
        if (on) {
            if (note == seq.at(seq_counter).get_value()) {
                if (seq_counter == 0) {
                    seq_started = std::chrono::steady_clock::now();
                }
                seq_counter++;
                SPDLOG_DEBUG("CMDSQ next seq[note={},on={},seq_counter={},context={}]", note, on, seq_counter, context);
                return false;
            }
            if (seq_counter > 0 && note == seq.at(seq_counter - 1).get_value()) {
                // Syntakt sends the same note twice
                return false;
            }
        }
    }
    seq_counter = 0;
    SPDLOG_DEBUG("CMDSQ reset seq[note={},on={},context={}]", note, on, context);
    return false;
}

int CommandSeq::get_context() const {
    return context;
}