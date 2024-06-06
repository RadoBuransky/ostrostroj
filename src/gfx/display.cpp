#define SPDLOG_ACTIVE_LEVEL 1

#include "common.hpp"
#include "display.hpp"

constexpr RGB palette_off = {0, 0, 0};
constexpr RGB palette_red = {RGB_MAX, 0, 0};
constexpr RGB palette_green = {0, RGB_10, 0};
constexpr RGB palette_blue = {0, 0, RGB_01};

constexpr RGB palette_yellow = {RGB_MAX, RGB_10, 0};
constexpr RGB palette_magenta = {RGB_MAX, 0, RGB_01};
constexpr RGB palette_cyan = {0, RGB_05, RGB_05};

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

void MainScreen::draw_loop(Point pos, TrackState& track_state, unicorn_hat_mini_canvas& canvas) {
    if (track_state != Playing) {
        return;
    }
    canvas.at(pos.x + 9).at(pos.y) = palette_playing;
    canvas.at(pos.x + 9).at(pos.y + 1) = palette_yellow;
    canvas.at(pos.x + 10).at(pos.y) = palette_playing;
    canvas.at(pos.x + 10).at(pos.y + 1) = palette_playing;
}

void MainScreen::draw_songs(unicorn_hat_mini_canvas& canvas) {
    uint count = std::min(song_count, (uint)2*9);
    for (uint i = 0; i < count; i++) {
        canvas.at(i % 9).at(1 + (i / 9)) = i <= song_index ? palette_magenta : palette_cyan;
    }
}

void MainScreen::draw_one_shots(unicorn_hat_mini_canvas& canvas) {
    for (uint i = 0; i < one_shots.size(); i++) {
        uint x = (i / 2) * 2;
        uint y = 4 + ((i % 2) * 2);
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
    }
}

void MainScreen::draw_patterns(unicorn_hat_mini_canvas& canvas) {
    uint count = std::min(pattern_count, (uint)8);
    for (uint i = 0; i < count; i++) {
        canvas.at((UNICORN_HAT_MINI_COLS - count) + i).at(5) = (i == pattern_index) ? palette_playing : palette_cyan;
    }
}

void MainScreen::draw_pattern_seq(unicorn_hat_mini_canvas& canvas) {
    uint count = std::min(pattern_seq_count, (uint)8);
    for (uint i = 0; i < count; i++) {
        canvas.at((UNICORN_HAT_MINI_COLS - count) + i).at(6) = (i <= pattern_seq_index) ? palette_playing : palette_cyan;
    }    
}

MainScreen::MainScreen():
    changed(true),
    booting(true),
    song_duration(0),
    pattern_duration(0),
    loops(),
    one_shots(),
    pattern_seq_count(0),
    pattern_seq_index(0) {
    loops.fill(Off);
    one_shots.fill(Off);
}

bool MainScreen::draw(unicorn_hat_mini_canvas& canvas) {
    if (!changed) {
        return false;
    }
    changed = false;
    canvas.fill({0,0,0});
    if (booting) {
        canvas.at(0).at(0) = palette_red;
        canvas.at(1).at(0) = palette_red;
        canvas.at(2).at(0) = palette_red;
        booting = false;
        changed = true;
    } else {
        draw_song_and_pattern_duration(canvas);
        draw_loops(canvas);
        draw_songs(canvas);
        draw_one_shots(canvas);
        draw_patterns(canvas);
        draw_pattern_seq(canvas);
    }
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

void MainScreen::set_loop_state(size_t loop_index, TrackState state) {
    if (loop_index < loops.size()) {
        loops.at(loop_index) = state;
        changed = true;
    }
}

void MainScreen::all_loops_off() {
    loops.fill(Off);
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

void MainScreen::set_pattern_seq_count(uint _count) {    
    pattern_seq_count = _count;
    changed = true;
}

void MainScreen::set_pattern_seq_index(uint _index) {
    pattern_seq_index = _index;
    changed = true;
}

Display::Display(std::chrono::milliseconds _refresh):
    refresh(_refresh),
    unicorn_hat_mini(),
    main_screen(),
    next_refresh() {
    tick(true);
}

void Display::tick(bool force) {
    if (!force && std::chrono::steady_clock::now() < next_refresh) {
        return;
    }
    next_refresh = std::chrono::steady_clock::now() + refresh;
    main_screen.draw(unicorn_hat_mini.get_canvas());
    unicorn_hat_mini.show();
}

MainScreen& Display::get_main_screen() {
    return main_screen;
}