#pragma once

#include <jack/jack.h>
#include "sample.hpp"

class Filter {
    public:
        virtual bool pop(jack_default_audio_sample_t& result) = 0;
};

class SampleFilter: Filter {
    public:
        SampleFilter(SampleReader sampleReader);
        virtual ~SampleFilter();

        bool pop(jack_default_audio_sample_t& result);
};