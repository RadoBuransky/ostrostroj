#pragma once

#include <samplerate.h>

class Warp {
    private:
        const size_t channels;
        std::array<float, 2> input_frame;
        size_t input_frame_pos;
        std::array<float, 32> output;
        size_t output_samples_gen;
        SRC_STATE* src_state;
        SRC_DATA src_data;
        float ratio;
        SRC_STATE* init_src_state(size_t channels);
    public:
        Warp(size_t _channels);
        virtual ~Warp();
        bool pushnpop(float &sample);
        bool pop(float &sample);
};