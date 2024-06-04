#pragma once

#include "clip.hpp"

class ClipPlayer {
    private:
        const Clip& clip;
        const bool loop;
        std::reference_wrapper<ClipBlock> block;
        const float* current_frame;
        const float* end_frame;
        long position;
        void update_pointers(ClipBlock& _block);
    public:
        ClipPlayer(Clip& _clip, bool _loop);
        virtual ~ClipPlayer() = default;
        inline bool pop(float& sample) {
            // TODO: Fade-in if loop
            if (current_frame < end_frame) {
                sample = *current_frame;
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