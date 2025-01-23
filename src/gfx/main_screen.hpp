#pragma once

#include "unicornhatmini.hpp"
#include "screen.hpp"
#include <chrono>

struct LoopState {
    bool muted;
    float saturation;
    bool grabbed;
};

enum TrackState {
    Off = 0,
    Muted,
    Playing
};

class MainScreen : public Screen {
    private:
        std::atomic_bool changed;
        std::array<LoopState, 6> loops;
        uint song_count;
        uint active_song_index;
        uint selected_song_index;
        uint pattern_count;
        uint active_pattern_index;
        uint selected_pattern_index;
        float pattern_fade_mix;
        uint pattern_position;
        bool clock_on;
        bool playing;
        std::chrono::steady_clock::duration playback_duration;
        void draw_clock();
        void draw_loop(uint8_t col, uint8_t row, LoopState loop_state);
    public:
        MainScreen(Canvas& _canvas);
        virtual ~MainScreen() = default;

        virtual bool draw();

        void set_loop_state(size_t loop_index, bool muted, float saturation);
        void set_loop_grabbed(size_t loop_index, bool grabbed);
        void all_loops_off();
        void set_song_count(uint _song_count);
        void set_active_song_index(uint _song_index);
        void set_selected_song_index(uint _song_index);
        void set_pattern_count(uint _pattern_count);
        void set_active_pattern_index(uint _pattern_index);
        void set_selected_pattern_index(uint _pattern_index);
        void set_pattern_fade(float _mix);
        void set_pattern_position(float _pattern_position);
        void set_clock(bool _on);
        void set_playing(bool _playing);
        void set_playback_duration(std::chrono::steady_clock::duration _playback_duration);
};