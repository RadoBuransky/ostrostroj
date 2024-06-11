#pragma once

class Warp {
    private:
        const size_t channels;
        const uint8_t track_number;
        std::array<float, 32> input_samples; // Must be at least 2 frames
        float* input_samples_pos;
        std::array<float, 256> output_samples; // Must be at big enough to hold all warped input samples at once
        size_t output_samples_gen;
        SRC_STATE* src_state;
        SRC_DATA src_data;
        double ratio; // -1.0 = half speed, 1.0 = double speed
        double ratio_accumulator; // positive=dragging, negative=pushing
        std::random_device random;
        std::default_random_engine random_engine;
        std::uniform_int_distribution<uint> target_change_dist;
        std::uniform_real_distribution<double> step_dist;
        double step_size;
        void update_ratio();
        double generate_step_size();
        SRC_STATE* init_src_state(size_t channels);
    public:
        Warp(size_t _channels, uint8_t _track_number);
        virtual ~Warp();
        bool pushnpop(float &sample);
        bool pop(float &sample);
};