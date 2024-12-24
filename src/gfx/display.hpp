#pragma once

#include "unicornhatmini.hpp"
#include "pattern.hpp"

constexpr RGB palette_off = {0, 0, 0};
constexpr RGB palette_red = {RGB_MAX, 0, 0};
constexpr RGB palette_green = {0, RGB_10, 0};
constexpr RGB palette_blue = {0, 0, RGB_01};
constexpr RGB palette_white = {RGB_MAX, RGB_25, RGB_25};
constexpr RGB palette_yellow = {RGB_MAX, RGB_10, 0};
constexpr RGB palette_magenta = {RGB_MAX, 0, RGB_01};
constexpr RGB palette_cyan = {0, RGB_05, RGB_05};

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

struct Point {
    uint8_t x;
    uint8_t y;
};

class Screen {
    public:
        virtual ~Screen() = default;
        virtual bool draw(unicorn_hat_mini_canvas& canvas) = 0;
};

class MainScreen : public Screen {
    public:
        static const size_t ONE_SHOT_COUNT = 10;

    private:
        std::atomic_bool changed;

        std::chrono::steady_clock::duration song_duration;
        std::chrono::steady_clock::duration pattern_duration;

        std::array<LoopState, 6> loops;
        std::array<TrackState, ONE_SHOT_COUNT> one_shots;

        uint song_count;
        uint song_index;

        uint pattern_count;
        uint pattern_index;

        void draw_song_and_pattern_duration(unicorn_hat_mini_canvas& canvas);
        void draw_loops(unicorn_hat_mini_canvas& canvas);
        void draw_loop(Point pos, LoopState loop, unicorn_hat_mini_canvas& canvas);
        void draw_one_shots(unicorn_hat_mini_canvas& canvas);
        void draw_songs(unicorn_hat_mini_canvas& canvas);
        void draw_patterns(unicorn_hat_mini_canvas& canvas);
    public:
        MainScreen();
        virtual ~MainScreen() = default;

        virtual bool draw(unicorn_hat_mini_canvas& canvas);

        void set_song_duration(std::chrono::steady_clock::duration _song_duration);
        void set_pattern_duration(std::chrono::steady_clock::duration _pattern_duration);

        void set_loop_state(size_t loop_index, bool muted, float saturation);
        void set_loop_grabbed(size_t loop_index, bool grabbed);
        void all_loops_off();

        void set_one_shot_state(size_t one_shot_index, TrackState state);
        void all_one_shots_off();

        void set_song_count(uint _song_count);
        void set_song_index(uint _song_index);

        void set_pattern_count(uint _pattern_count);
        void set_pattern_index(uint _pattern_index);
};

class SystemScreen : public Screen {
    private:
        bool init;
        float mem_usage;
    public:
        SystemScreen();
        virtual ~SystemScreen() = default;
        virtual bool draw(unicorn_hat_mini_canvas& canvas);
        void set_mem_usage(float _mem_usage);
};

class Display {
    private:
        const std::chrono::milliseconds refresh;
        UnicornHatMini unicorn_hat_mini;
        MainScreen main_screen;
        SystemScreen system_screen;
        std::reference_wrapper<Screen> active_screen;
        std::chrono::time_point<std::chrono::steady_clock> next_refresh;

    public:
        Display(std::chrono::milliseconds _refresh);
        virtual ~Display() = default;

        /**
         * Call this as often as you want, it won't refresh the screen faster than "refresh" period.
        */
        void tick(bool force);

        MainScreen& get_main_screen();
        SystemScreen& get_system_screen();
        void set_active_screen(Screen& _screen);
};