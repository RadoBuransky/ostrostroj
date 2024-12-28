#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "main_screen.hpp"

constexpr RGB palette_playing = palette_red;
constexpr RGB palette_muted = palette_blue;

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
    loops(),
    one_shots(),
    pattern_frames(0),
    pattern_position(0),
    clock_on(false) {
    loops.fill({true, 0.0, false});
    one_shots.fill(Off);
}

bool MainScreen::draw(unicorn_hat_mini_canvas& canvas) {
    if (!changed) {
        return false;
    }
    changed = false;
    canvas.fill({0,0,0});
    // TODO: Draw pattern position
    canvas.at(0).at(UNICORN_HAT_MINI_ROWS - 1) = (clock_on) ? palette_green : palette_off;
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

void MainScreen::set_pattern_frames(snd_pcm_uframes_t _pattern_frames) {
    pattern_frames = _pattern_frames;
}

void MainScreen::set_pattern_position(snd_pcm_uframes_t _pattern_position) {
    pattern_position = _pattern_position;
}

void MainScreen::blink_clock() {
    clock_on = !clock_on;
    changed = true;
}