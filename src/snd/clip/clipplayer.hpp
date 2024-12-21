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
        const short* current_frame;
        const short* end_frame;
        long position;
        bool draining;
        bool muted;
        void update_pointers(ClipBlock& _block);
        void fade_out();
        void fade_in();
    public:
        ClipPlayer(Clip& _clip, bool _loop, snd_pcm_uframes_t _latency_frames, bool predelay, bool _muted);
        virtual ~ClipPlayer() = default;
        inline bool pop(float& sample) {
            if (fade > fade_samples) {
                // Predelay
                fade--;
                sample = 0.0;
                return true;
            }
            if (current_frame < end_frame) {
                sample = ((float)*current_frame / (float)SHRT_MAX);
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
                current_frame++;
                position++;
                if (muted) {
                    sample = 0.0;
                }
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
        void set_muted(bool _muted);
        bool get_muted();
        Clip& get_clip();
};