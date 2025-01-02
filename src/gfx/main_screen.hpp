#pragma once

#include "unicornhatmini.hpp"
#include "screen.hpp"

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
        uint song_index;

        uint pattern_count;
        uint pattern_index;

        // Percentage 0.0 - 1.0
        float pattern_position;

        bool clock_on;

        void draw_loops(unicorn_hat_mini_canvas& canvas);
        void draw_loop(Point pos, LoopState loop, unicorn_hat_mini_canvas& canvas);
        void draw_songs(unicorn_hat_mini_canvas& canvas);
        void draw_patterns(unicorn_hat_mini_canvas& canvas);
    public:
        MainScreen();
        virtual ~MainScreen() = default;

        virtual bool draw(unicorn_hat_mini_canvas& canvas);

        void set_loop_state(size_t loop_index, bool muted, float saturation);
        void set_loop_grabbed(size_t loop_index, bool grabbed);
        void all_loops_off();

        void set_song_count(uint _song_count);
        void set_song_index(uint _song_index);

        void set_pattern_count(uint _pattern_count);
        void set_pattern_index(uint _pattern_index);

        void set_pattern_position(float _pattern_position);

        void blink_clock();
};