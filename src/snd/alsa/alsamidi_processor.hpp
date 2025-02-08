#pragma once
#include "volmod_processor.hpp"

class AlsaMidiProcessor {
    private:
        VolModProcessor volMod;
    public:
        AlsaMidiProcessor();
        virtual ~AlsaMidiProcessor() = default;
        std::vector<snd_seq_event_t> process(snd_seq_event_t &event);
        VolModProcessor& get_volmod_processor();
};