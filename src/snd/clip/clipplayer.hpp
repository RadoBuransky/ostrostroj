#pragma once

#include "clip.hpp"

class ClipPlayer {
    private:
        Clip& clip;
        const bool loop;
        const int32_t latency_samples;
        const int32_t fade_samples;
        int32_t fade;
        std::reference_wrapper<ClipBlock> block;
        const short* current_sample;
        const short* end_sample;
        uint32_t position;
        bool draining;
        void update_pointers(ClipBlock& _block);
        void fade_out();
        void fade_in();
    public:
        ClipPlayer(Clip& _clip, bool _loop, snd_pcm_uframes_t _latency_frames, bool predelay);
        virtual ~ClipPlayer() = default;
        inline bool pop(float& sample) {
            if (fade > fade_samples) {
                // Predelay
                fade--;
                sample = 0.0;
                return true;
            }
            if (current_sample < end_sample) {
                sample = ((float)*current_sample / (float)SHRT_MAX);
                if (fade != 0) {
                    if (fade > 0) {
                        // Fade in
                        sample *= (float)(fade_samples - fade) / (float)fade_samples;
                        fade--;
                    } else {
                        if (fade > -fade_samples) {
                            // Fade out
                            sample *= -1.0 * (float)fade / (float)fade_samples;
                        }
                        fade++;
                        if (fade == 0 && draining) {
                            // Draining done
                            return false;
                        }
                    }                    
                }
                current_sample++;
                position++;
                return true;        
            }
            if (block.get().has_next()) {
                block = std::ref(block.get().get_next());
            } else {
                if (!loop) {
                    return false;
                }
                block = std::ref(clip.get_head());
                position = 0;
            }
            update_pointers(block.get());
            return pop(sample);
        }
        void drain();
        Clip& get_clip();
        float get_position();
};