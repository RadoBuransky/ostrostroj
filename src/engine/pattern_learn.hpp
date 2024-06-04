#pragma once

#include "pattern.hpp"

class PatternLearn {
    private:
        Pattern& pattern;
        uint8_t first_clock;
        uint8_t get_step(unsigned int clock);
        bool check(unsigned int clock);
    public:
        PatternLearn(Pattern& _pattern);
        virtual ~PatternLearn() = default;
        void note(uint8_t note, bool on, unsigned int clock);
        void controller(unsigned int param, signed int value, unsigned int clock);
};