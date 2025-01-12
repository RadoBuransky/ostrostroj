#pragma once

#include "song.hpp"
#include "pattern.hpp"
#include "clipplayer.hpp"

class Engine;

class ProgramChange {
    private:
        Engine& engine;
        uint8_t selected_song_index;
        uint8_t selected_pattern_index;
        std::vector<std::reference_wrapper<ClipPlayer>> active_clip_players;
        std::vector<std::reference_wrapper<ClipPlayer>> selected_clip_players;
        bool fading;
        void update_display();
        void set_gain(float gain, std::vector<std::reference_wrapper<ClipPlayer>>& clip_players);
    public:
        ProgramChange(Engine& _engine);
        virtual ~ProgramChange() = default;
        void select_next();
        void select_prev();
        void on_fader(float mix);
        void on_program_changed(bool running);

        // TODO: transition starts when fading encoder is set to 0 (not pad) - clips are added to tracks
        // TODO: transition finishes when fading encoder is set to 100% - clips are removed
        // TODO: how to cancel transition?
};
