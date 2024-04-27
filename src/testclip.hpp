#pragma once

#include "clip.hpp"

/**
 * Useful for testing seamless looping.
*/
class RampDownClip: public Clip {
    private:
        static constexpr int blocks = 2;
        BufferClipBlock head;
        RampDownClip();
        void init(float total_frames, int pos, ClipBlock& block);
    public:
        static RampDownClip& get() {
            static RampDownClip instance;
            return instance;
        }
        RampDownClip(RampDownClip const&) = delete;
        void operator=(RampDownClip const&) = delete;
        virtual ClipBlock& get_head();
};