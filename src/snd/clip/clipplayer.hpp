#pragma once

#include <alsa/asoundlib.h>
#include "clip.hpp"

class ClipPlayer {
    private:
        const Clip& clip;
        const bool loop;
        const int32_t latency_samples;
        const int32_t fade_samples;
        int32_t fade;
        std::reference_wrapper<ClipBlock> block;
        const float* current_frame;
        const float* end_frame;
        long position;
        void update_pointers(ClipBlock& _block);
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
            if (current_frame < end_frame) {
                sample = *current_frame;
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
                        if (fade == 0) {
                            // Draining done
                            return false;
                        }
                    }                    
                }
                current_frame++;
                position++;
                return true;        
            }
            if (block.get().has_next()) {
                block = std::ref(block.get().get_next());
            } else {
                if (!loop) {
                    return false;
                }
                // SPDLOG_DEBUG("Lopp restart. [path={}, this=0x{:x}, clip=0x{:x}, current_frame=0x{:x}, end_frame=0x{:x}, block=0x{:x}]",
                //     clip.get_path().c_str(), reinterpret_cast<intptr_t>(this), reinterpret_cast<intptr_t>(&clip),
                //     reinterpret_cast<intptr_t>(current_frame.load()), reinterpret_cast<intptr_t>(end_frame.load()),
                //     reinterpret_cast<intptr_t>(&block.get()));
                block = std::ref(clip.get_head());
                position = 0;
            }
            update_pointers(block.get());
            return pop(sample);
        }
        void drain();
};