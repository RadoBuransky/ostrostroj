#pragma once

#include "clip.hpp"

class ClipPlayer {
    private:
        Clip& clip;
        std::reference_wrapper<ClipBlock> block;
        const short* current_sample;
        const short* end_sample;
        uint32_t position;
        bool paused;
        float gain;
        int32_t skip_samples;
        void update_pointers(ClipBlock& _block);
        void fade_out();
        void fade_in();
        inline bool next_frame(float& sample) {
            if (current_sample < end_sample) {
                sample = gain * ((float)*current_sample / (float)SHRT_MAX);
                current_sample++;
                position++;
                return true;
            }
            if (block.get().has_next()) {
                block = std::ref(block.get().get_next());
            } else {
                block = std::ref(clip.get_head());
                position = 0;
            }
            update_pointers(block.get());
            return pop(sample);
        }
    public:
        ClipPlayer(Clip& _clip);
        virtual ~ClipPlayer();
        inline bool pop(float& sample) {
            while (skip_samples > 0) {
                if (!next_frame(sample)) {
                    return false;
                }
                skip_samples--;
            }
            if (paused) {
                sample = 0.0;
                return true;
            }
            return next_frame(sample);
        }
        void drain();
        Clip& get_clip();
        float get_position();
        void set_paused(bool _paused);
        bool is_paused();
        void set_gain(float _gain);
        float get_gain();
        void skip(std::chrono::steady_clock::duration period);
};