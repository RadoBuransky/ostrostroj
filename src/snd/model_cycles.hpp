#pragma once

#include <alsa/asoundlib.h>

class ModelCycles {
    public:
        static constexpr size_t MODEL_CYCLES_TRACK_COUNT = 6;
        ModelCycles();
        virtual ~ModelCycles() = default;
        snd_seq_event_t mute_track(size_t track_number, bool muted);
};