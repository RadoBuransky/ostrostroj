#define SPDLOG_ACTIVE_LEVEL 1

#include "common.hpp"
#include "pattern_learn.hpp"
#include "engine.hpp"

static constexpr uint8_t T1_NOTE = 36; // TODO: Should be C3 note
static constexpr uint8_t L1_PARAM = 111; // CC #111
static constexpr uint8_t STEP_SIZE = 6; // 1/8th note (https://en.wikipedia.org/wiki/MIDI_beat_clock)
static constexpr uint8_t MAX_STEPS = 16;
static constexpr int MIN_SATURATION = 1;
static constexpr int MAX_SATURATION = 127;

uint8_t PatternLearn::get_step(unsigned int clock) {
    return (clock - first_clock) / STEP_SIZE;
}

bool PatternLearn::check(unsigned int clock) {
    if (pattern.get_learned()) {
        return false;
    }
    if (first_clock == 0) {
        first_clock = clock;
    }
    if (clock - first_clock >= MAX_STEPS * STEP_SIZE) {
        pattern.set_learned();
        return false;
    }
    return true;
}

PatternLearn::PatternLearn(Pattern& _pattern):
    pattern(_pattern),
    first_clock(0) {
}

void PatternLearn::note(uint8_t note, bool on, unsigned int clock) {
    if (!check(clock)) {
        return;
    }
    if (!on || note < T1_NOTE || note >= T1_NOTE + PATTERN_MUTES) {
        return;
    }
    std::vector<bool>& mutes = pattern.get_mutes().at(note - T1_NOTE);
    uint8_t step = get_step(clock);
    size_t old_size = mutes.size();
    if (step >= mutes.size()) {
        mutes.resize(step + 1);
        for (size_t i = old_size; i < step; i++) {
            mutes.at(i) = true;
        }
        mutes.at(step) = false;
    }
    SPDLOG_DEBUG("PRJKT note learned [note={},on={},clock={},step={}]", note, on, clock, step);
}

void PatternLearn::controller(unsigned int param, signed int value, unsigned int clock) {
    if (!check(clock)) {
        return;
    }
    if (param < L1_PARAM || param >= L1_PARAM + ENGINE_LOOP_TRACKS) {
        return;
    }
    uint8_t track = (param - L1_PARAM) + 1;
    for (PatternLoop& loop : pattern.get_loops()) {
        if (loop.track == track) {
            uint8_t step = get_step(clock);
            size_t old_size = loop.seq.size();
            if (step >= loop.seq.size()) {
                loop.seq.resize(step + 1);
                for (size_t i = old_size; i < step; i++) {
                    loop.seq.at(i).muted = true;
                }
                PatternLoopSeq& seq = loop.seq.at(step);
                if (value < MIN_SATURATION) {
                    seq.muted = true;
                } else {
                    seq.muted = false;
                    seq.saturation = ((float)(value - MIN_SATURATION)) / (float)(MAX_SATURATION - MIN_SATURATION);
                }
            }
            SPDLOG_DEBUG("PRJKT controller learned [param={},value={},clock={},step={}]", param, value, clock, step);
            return;
        }
    }
}