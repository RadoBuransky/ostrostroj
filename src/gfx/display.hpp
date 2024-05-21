#pragma once

#include "unicornhatmini.hpp"

enum TrackState {
    Off = 0,
    Muted,
    Playing
};

struct Point {
    uint8_t x;
    uint8_t y;
};

class MainScreen {
    private:
        std::atomic_bool changed;

        std::chrono::seconds song_duration;
        std::chrono::seconds pattern_duration;

        std::array<TrackState, 6> loops;
        std::array<TrackState, 10> one_shots;

        uint song_count;
        uint song_index;

        uint pattern_count;
        uint pattern_index;

        uint pattern_seq_count;
        uint pattern_seq_index;

        void draw_song_and_pattern_duration(unicorn_hat_mini_canvas& canvas);
        void draw_loops(unicorn_hat_mini_canvas& canvas);
        void draw_loop(Point pos, TrackState& track_state, unicorn_hat_mini_canvas& canvas);
        void draw_one_shots(unicorn_hat_mini_canvas& canvas);
        void draw_songs(unicorn_hat_mini_canvas& canvas);
        void draw_patterns(unicorn_hat_mini_canvas& canvas);
        void draw_pattern_seq(unicorn_hat_mini_canvas& canvas);
    public:
        MainScreen();
        virtual ~MainScreen();

        bool draw(unicorn_hat_mini_canvas& canvas);

        void set_song_duration(std::chrono::seconds _song_duration);
        void set_pattern_duration(std::chrono::seconds _pattern_duration);

        void set_loop_playing(uint _number);
        void set_loop_muted(uint _number);
        void set_loop_off(uint _number);

        void set_one_shot_playing(uint _number);
        void set_one_shot_muted(uint _number);
        void set_one_shot_off(uint _number);

        void set_pattern_seq_count(uint _count);
        void set_pattern_seq_index(uint _index);
};

class Display {
    private:
        const std::chrono::milliseconds refresh;
        UnicornHatMini unicorn_hat_mini;
        MainScreen main_screen;
        std::chrono::time_point<std::chrono::steady_clock> next_refresh;

    public:
        Display(std::chrono::milliseconds _refresh);
        virtual ~Display();

        /**
         * Call this as often as you want, it won't refresh the screen faster than "refresh" period.
        */
        void tick();
        MainScreen& get_main_screen();
};