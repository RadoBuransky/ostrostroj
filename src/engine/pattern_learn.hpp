#pragma once

#include "pattern.hpp"

class PatternLearn {
    private:
        Pattern& pattern;
        uint first_clock;
        uint8_t step;
        void update_step(uint clock);
        bool check_step_and_set_learned();
    public:
        PatternLearn(Pattern& _pattern);
        virtual ~PatternLearn() = default;
        bool valid_note(uint8_t note);
        void note(uint8_t note, bool on, uint clock);
        bool valid_controller(uint param);
        void controller(uint param, signed int value, uint clock);
        uint8_t get_step();
};