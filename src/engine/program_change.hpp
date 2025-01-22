#pragma once

#include "song.hpp"
#include "pattern.hpp"
#include "clipplayer.hpp"

class Engine;

class ProgramChange {
    private:
        Engine& engine;
        std::reference_wrapper<Song> selected_song;
        std::reference_wrapper<Pattern> selected_pattern;
        std::vector<std::reference_wrapper<ClipPlayer>> active_clip_players;
        std::vector<std::reference_wrapper<ClipPlayer>> selected_clip_players;
        bool fading;
        bool resume_selected_clip_players;
        void update_display();
        void set_gain(float gain, std::vector<std::reference_wrapper<ClipPlayer>>& clip_players);
    public:
        ProgramChange(Engine& _engine);
        virtual ~ProgramChange() = default;
        void select_next();
        void select_prev();
        void on_fader(float mix);
        void on_program_changed(bool running);
        void on_quarter_note_clock();
};
