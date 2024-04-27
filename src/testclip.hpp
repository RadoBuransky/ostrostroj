#pragma once

#include "clip.hpp"

/**
 * Useful for testing seamless looping.
*/
class RampDownClip: public Clip {
    public:
        RampDownClip();
        virtual ~RampDownClip() = default;
};