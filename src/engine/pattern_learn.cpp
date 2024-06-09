#define SPDLOG_ACTIVE_LEVEL 2

#include "common.hpp"
#include "pattern_learn.hpp"
#include "engine.hpp"

static constexpr uint8_t T1_NOTE = 36; // C3
static constexpr uint8_t L1_PARAM = 111; // CC #111
static constexpr uint8_t STEP_SIZE = 6; // 1/8th note (https://en.wikipedia.org/wiki/MIDI_beat_clock)
static constexpr uint8_t MAX_STEPS = 16;
static constexpr int MIN_SATURATION = 1;
static constexpr int MAX_SATURATION = 127;

uint8_t PatternLearn::get_step(uint clock) {
    if (first_clock == 0) {
        first_clock = clock;
    }
    return (clock - first_clock) / STEP_SIZE;
}

bool PatternLearn::check_step_and_set_learned(uint8_t step) {
    if (pattern.get_learned()) {
        return false;
    }
    if (step >= MAX_STEPS) {
        SPDLOG_DEBUG("PRJKT check learned[step={},first_clock={}]", step, first_clock);
        pattern.set_learned();
        return false;
    }
    return true;
}

PatternLearn::PatternLearn(Pattern& _pattern):
    pattern(_pattern),
    first_clock(0) {
}

bool PatternLearn::valid_note(uint8_t note) {
    return (note >= T1_NOTE) && (note < (T1_NOTE + PATTERN_MUTES));
}

void PatternLearn::note(uint8_t note, bool on, uint clock) {
    uint8_t step = get_step(clock);
    if (!check_step_and_set_learned(step)) {
        return;
    }
    if (!on || !valid_note(note)) {
        return;
    }
    std::vector<bool>& mutes = pattern.get_mutes().at(note - T1_NOTE);
    size_t old_size = mutes.size();
    if (step >= mutes.size()) {
        mutes.resize(step + 1);
        for (size_t i = old_size; i < step; i++) {
            mutes.at(i) = true;
        }
    }
    mutes.at(step) = false;
    pattern.update_seq_count();
    SPDLOG_DEBUG("PRJKT note learned [note={},on={},clock={},step={}]", note, on, clock, step);
}

bool PatternLearn::valid_controller(uint param) {
    return (param >= L1_PARAM) && (param < (L1_PARAM + ENGINE_LOOP_TRACKS));
}

void PatternLearn::controller(uint param, signed int value, uint clock) {
    uint8_t step = get_step(clock);
    if (!check_step_and_set_learned(step)) {
        return;
    }
    if (!valid_controller(param)) {
        return;
    }
    uint8_t track_number = (param - L1_PARAM) + 1;
    for (PatternLoop& loop : pattern.get_loops()) {
        if (loop.track_number == track_number) {
            size_t old_size = loop.seq.size();
            if (step >= loop.seq.size()) {
                loop.seq.resize(step + 1);
                for (size_t i = old_size; i < step; i++) {
                    loop.seq.at(i).muted = true;
                }
            }
            PatternLoopSeq& seq = loop.seq.at(step);
            if (value < MIN_SATURATION) {
                seq.muted = true;
            } else {
                seq.muted = false;
                seq.saturation = ((float)(value - MIN_SATURATION)) / (float)(MAX_SATURATION - MIN_SATURATION);
            }
            pattern.update_seq_count();
            SPDLOG_DEBUG("PRJKT controller learned [param={},value={},clock={},step={},track_number={},muted={},saturation={}]",
                param, value, clock, step, track_number, seq.muted, seq.saturation);
            return;
        }
    }
}