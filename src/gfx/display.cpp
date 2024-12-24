#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "display.hpp"

constexpr RGB palette_playing = palette_red;
constexpr RGB palette_muted = palette_blue;

void MainScreen::draw_song_and_pattern_duration(unicorn_hat_mini_canvas& canvas) {
    size_t song_width = std::min((size_t)std::chrono::duration_cast<std::chrono::minutes>(song_duration).count(), (size_t)9);
    for (size_t i = 0; i < song_width; i++) {
        canvas.at(i).at(0) = palette_playing;
    }
    size_t pattern_width = std::min((size_t)std::chrono::duration_cast<std::chrono::minutes>(pattern_duration).count(), (size_t)8);
    for (size_t i = 0; i < pattern_width; i++) {
        canvas.at((UNICORN_HAT_MINI_COLS - 1) - i).at(4) = palette_playing;
    }
}

void MainScreen::draw_loops(unicorn_hat_mini_canvas& canvas) {
    draw_loop({0, 0}, loops[0], canvas);
    draw_loop({2, 0}, loops[1], canvas);
    draw_loop({4, 0}, loops[2], canvas);
    draw_loop({6, 0}, loops[3], canvas);
    draw_loop({0, 2}, loops[4], canvas);
    draw_loop({2, 2}, loops[5], canvas);
}

void MainScreen::draw_loop(Point pos, LoopState loop, unicorn_hat_mini_canvas& canvas) {
    static const std::array<std::array<uint8_t, 2>, 4> shape = {{
        {9,1},
        {9,0},
        {10,0},
        {10,1}
    }};
    RGB color;
    float compensated_saturation = std::max(loop.saturation, (float)0.1);
    RGB point_color;
    if (loop.muted) {
        color = palette_muted;
        point_color = palette_muted;
    } else {        
        color = (loop.grabbed ? palette_white : palette_playing) * compensated_saturation;
        point_color = (loop.grabbed ? palette_magenta : palette_yellow) * compensated_saturation;
    }
    uint8_t yellow_pos = std::min((uint8_t)3, (uint8_t)(loop.saturation * 4.0));
    for (size_t pixel = 0; pixel < shape.size(); pixel++) {
        const std::array<uint8_t, 2>& xy = shape.at(pixel);
        canvas.at(xy.at(0) + pos.x).at(xy.at(1) + pos.y) = (yellow_pos == pixel) ? point_color : color;
    }
    SPDLOG_DEBUG("DSPLY draw_loop[track_state={}]", track_state);
}

void MainScreen::draw_songs(unicorn_hat_mini_canvas& canvas) {
    uint count = std::min(song_count, (uint)2*9);
    for (uint i = 0; i < count; i++) {
        canvas.at(i % 9).at(1 + (i / 9)) = i <= song_index ? palette_magenta : palette_cyan;
    }
    SPDLOG_DEBUG("DSPLY draw_songs[song_count={},song_index={}]", song_count, song_index);
}

void MainScreen::draw_one_shots(unicorn_hat_mini_canvas& canvas) {
    const size_t row_one_shots = ONE_SHOT_COUNT / 2;
    for (uint i = 0; i < one_shots.size(); i++) {
        size_t x = (i % row_one_shots) * 2;
        size_t y = (UNICORN_HAT_MINI_ROWS - 1) - ((i / row_one_shots) * 2);
        RGB color;
        switch (one_shots[i]) {
            case Muted:
                color = palette_cyan;
                break;
            case Playing:
                color = palette_playing;
                break;
            default:
                color = palette_off;
                break;
        }
        canvas.at(x).at(y) = color;
        SPDLOG_DEBUG("DSPLY draw_one_shots[i={},one_shots[i]={}]", i, one_shots[i]);
    }
}

void MainScreen::draw_patterns(unicorn_hat_mini_canvas& canvas) {
    uint count = std::min(pattern_count, (uint)8);
    for (uint i = 0; i < count; i++) {
        canvas.at((UNICORN_HAT_MINI_COLS - count) + i).at(5) = (i == pattern_index) ? palette_playing : palette_cyan;
    }
    SPDLOG_DEBUG("DSPLY draw_patterns[pattern_count={},pattern_index={}]", pattern_count, pattern_index);
}

MainScreen::MainScreen():
    changed(true),
    song_duration(0),
    pattern_duration(0),
    loops(),
    one_shots() {
    loops.fill({false, 0.0, false});
    one_shots.fill(Off);
}

bool MainScreen::draw(unicorn_hat_mini_canvas& canvas) {
    if (!changed) {
        return false;
    }
    changed = false;
    canvas.fill({0,0,0});
    draw_song_and_pattern_duration(canvas);
    draw_loops(canvas);
    draw_songs(canvas);
    draw_one_shots(canvas);
    draw_patterns(canvas);
    return true;
}

void MainScreen::set_song_duration(std::chrono::steady_clock::duration _song_duration) {
    song_duration = _song_duration;
    changed = true;
}

void MainScreen::set_pattern_duration(std::chrono::steady_clock::duration _pattern_duration) {
    pattern_duration = _pattern_duration;
    changed = true;
}

void MainScreen::set_loop_state(size_t loop_index, bool muted, float saturation) {
    if (loop_index < loops.size()) {
        LoopState& loopState = loops.at(loop_index);
        loopState.muted = muted;
        loopState.saturation = saturation;
        changed = true;
    }
}

void MainScreen::set_loop_grabbed(size_t loop_index, bool grabbed) {
    loops.at(loop_index).grabbed = grabbed;
    changed = true;
}

void MainScreen::all_loops_off() {
    loops.fill({false, 0.0, false});
}

void MainScreen::set_one_shot_state(size_t one_shot_index, TrackState state) {
    if (one_shot_index < one_shots.size()) {
        one_shots.at(one_shot_index) = state;
        changed = true;
    }
}

void MainScreen::all_one_shots_off() {
    one_shots.fill(Off);
}

void MainScreen::set_song_count(uint _song_count) {
    song_count = _song_count;
    changed = true;
}

void MainScreen::set_song_index(uint _song_index) {
    song_index = _song_index;
    changed = true;
}

void MainScreen::set_pattern_count(uint _pattern_count) {
    pattern_count = _pattern_count;
    changed = true;
}

void MainScreen::set_pattern_index(uint _pattern_index) {
    pattern_index = _pattern_index;
    changed = true;
}

SystemScreen::SystemScreen():
    init(true),
    mem_usage(0) {    
}

bool SystemScreen::draw(unicorn_hat_mini_canvas& canvas) {    
    if (init) {
        canvas.at(0).at(0) = palette_red;
        canvas.at(1).at(0) = palette_red;
        canvas.at(2).at(0) = palette_red;
        init = false;
        return true;
    }
    size_t mem_usage_cols = std::ceil(mem_usage*UNICORN_HAT_MINI_COLS);
    for (size_t x = 0; x < mem_usage_cols; x++) {
        canvas.at(x).at(0) = palette_cyan;
    }
    return true;
}

void SystemScreen::set_mem_usage(float _mem_usage) {
    mem_usage = std::max(0.0f, std::min(1.0f, _mem_usage));
}

Display::Display(std::chrono::milliseconds _refresh):
    refresh(_refresh),
    unicorn_hat_mini(),
    main_screen(),
    system_screen(),
    active_screen(system_screen),
    next_refresh() {
    tick(true);
}

void Display::tick(bool force) {
    if (!force && std::chrono::steady_clock::now() < next_refresh) {
        return;
    }
    next_refresh = std::chrono::steady_clock::now() + refresh;
    if (active_screen.get().draw(unicorn_hat_mini.get_canvas())) {
        unicorn_hat_mini.show();
    }
}

MainScreen& Display::get_main_screen() {
    return main_screen;
}

SystemScreen& Display::get_system_screen() {
    return system_screen;
}

void Display::set_active_screen(Screen& _screen) {
    active_screen = _screen;
    unicorn_hat_mini.clear();
}