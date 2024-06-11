#pragma once

#include "unicornhatmini.hpp"
#include "pattern.hpp"

struct LoopState {
    PatternLoopSeq seq;
    bool grabbed;
};

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
    public:
        static const size_t ONE_SHOT_COUNT = 10;

    private:
        std::atomic_bool changed;
        std::atomic_bool booting;

        std::chrono::steady_clock::duration song_duration;
        std::chrono::steady_clock::duration pattern_duration;

        std::array<LoopState, 6> loops;
        std::array<TrackState, ONE_SHOT_COUNT> one_shots;

        uint song_count;
        uint song_index;

        uint pattern_count;
        uint pattern_index;

        uint pattern_seq_count;
        uint pattern_seq_index;

        void draw_song_and_pattern_duration(unicorn_hat_mini_canvas& canvas);
        void draw_loops(unicorn_hat_mini_canvas& canvas);
        void draw_loop(Point pos, LoopState loop, unicorn_hat_mini_canvas& canvas);
        void draw_one_shots(unicorn_hat_mini_canvas& canvas);
        void draw_songs(unicorn_hat_mini_canvas& canvas);
        void draw_patterns(unicorn_hat_mini_canvas& canvas);
        void draw_pattern_seq(unicorn_hat_mini_canvas& canvas);
    public:
        MainScreen();
        virtual ~MainScreen() = default;

        bool draw(unicorn_hat_mini_canvas& canvas);

        void set_song_duration(std::chrono::steady_clock::duration _song_duration);
        void set_pattern_duration(std::chrono::steady_clock::duration _pattern_duration);

        void set_loop_state(size_t loop_index, PatternLoopSeq state);
        void set_loop_grabbed(size_t loop_index, bool grabbed);
        void all_loops_off();

        void set_one_shot_state(size_t one_shot_index, TrackState state);
        void all_one_shots_off();

        void set_song_count(uint _song_count);
        void set_song_index(uint _song_index);

        void set_pattern_count(uint _pattern_count);
        void set_pattern_index(uint _pattern_index);

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
        virtual ~Display() = default;

        /**
         * Call this as often as you want, it won't refresh the screen faster than "refresh" period.
        */
        void tick(bool force);
        MainScreen& get_main_screen();
};