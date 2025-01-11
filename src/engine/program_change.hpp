#pragma once

#include "song.hpp"
#include "pattern.hpp"

class Engine;

class ProgramChange {
    private:
        Engine& engine;
        uint8_t selected_song_index;
        uint8_t selected_pattern_index;
        void update_display();
    public:
        ProgramChange(Engine& _engine);
        virtual ~ProgramChange() = default;
        void select_next();
        void select_prev();
        void on_fader(float mix);
        void on_program_changed();

        // TODO: transition starts when fading encoder is set to 0 (not pad) - clips are added to tracks
        // TODO: transition finishes when fading encoder is set to 100% - clips are removed
        // TODO: how to cancel transition?
};
