#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "main_screen.hpp"

constexpr RGB palette_playing = palette_red;
constexpr RGB palette_muted = palette_blue;

MainScreen::MainScreen():
    changed(true),
    loops(),
    pattern_position(0.0),
    clock_on(false) {
    loops.fill({true, 0.0, false});
}

bool MainScreen::draw(Canvas& canvas) {
    if (!changed) {
        return false;
    }
    changed = false;
    canvas.clear();    
    canvas.point(0, UNICORN_HAT_MINI_ROWS - 1, (clock_on) ? palette_green : palette_off);
    return true;
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
    loops.fill({true, 0.0, false});
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

void MainScreen::set_pattern_position(float _pattern_position) {
    pattern_position = _pattern_position;
}

void MainScreen::blink_clock() {
    clock_on = !clock_on;
    changed = true;
}