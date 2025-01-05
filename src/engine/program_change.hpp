#pragma once

#include "engine.hpp"

class ProgramChange {
    private:
        Engine& engine;
        std::reference_wrapper<Song> selected_song;
        std::reference_wrapper<Pattern> selected_pattern;

    public:
        ProgramChange(Engine& _engine);
        virtual ~ProgramChange() = default;
        void on_change_selection(int delta);
        void on_fader(float mix);
        void on_program_changed();

        // TODO: transition starts when fading encoder is set to 0 (not pad) - clips are added to tracks
        // TODO: transition finishes when fading encoder is set to 100% - clips are removed
        // TODO: how to cancel transition?
};
