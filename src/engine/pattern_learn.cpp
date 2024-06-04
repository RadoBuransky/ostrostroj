#include "common.hpp"
#include "pattern_learn.hpp"

static constexpr uint8_t T1_NOTE = 35; // TODO: Should be C3 note
static constexpr uint8_t STEP_SIZE = 16;
static constexpr uint8_t MAX_STEPS = 16;

PatternLearn::PatternLearn(Pattern& _pattern):
    pattern(_pattern),
    first_clock(0) {
}

void PatternLearn::note(uint8_t note, bool on, unsigned int clock) {
    if (pattern.get_learned()) {
        return;
    }
    if (first_clock == 0) {
        first_clock = clock;
    }
    if (clock - first_clock >= MAX_STEPS * STEP_SIZE) {
        pattern.set_learned();
        return;
    }
    if (note < T1_NOTE || note >= T1_NOTE + PATTERN_MUTES) {
        return;
    }
    std::vector<bool> mutes = pattern.get_mutes().at(note - T1_NOTE);
    uint8_t step = (clock - first_clock) / STEP_SIZE;
    size_t old_size = mutes.size();
    if (step >= mutes.size()) {
        mutes.resize(step + 1);
        // Fill the gap
        for (size_t i = old_size; i < step; i++) {
            mutes.at(i) = true;
        }
        mutes.at(step) = !on;
    }
}

void PatternLearn::controller(unsigned int param, signed int value, unsigned int clock) {
    if (pattern.get_learned()) {
        return;
    }
    if (first_clock == 0) {
        first_clock = clock;
    }
    if (clock - first_clock >= MAX_STEPS * STEP_SIZE) {
        pattern.set_learned();
        return;
    }
}