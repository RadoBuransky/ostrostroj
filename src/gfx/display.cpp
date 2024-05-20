#include "common.hpp"
#include "display.hpp"

constexpr Point loops_top_left = {9, 0};
constexpr RGB loops_off_color = {0, 0, 0};
constexpr RGB loops_muted_color = {0, 0, RGB_01};
constexpr RGB loops_playing_color = {RGB_MAX, 0, 0};

void MainScreen::draw_loops(unicorn_hat_mini_canvas& canvas) {
    draw_loop({0, 0}, loops[0], canvas);
    draw_loop({2, 0}, loops[1], canvas);
    draw_loop({4, 0}, loops[2], canvas);
    draw_loop({6, 0}, loops[3], canvas);
    draw_loop({0, 2}, loops[4], canvas);
    draw_loop({2, 2}, loops[5], canvas);
}

void MainScreen::draw_loop(Point pos, TrackState& track_state, unicorn_hat_mini_canvas& canvas) {
    RGB color;
    switch (track_state) {
        case Off:
            color = loops_off_color;
            break;
        case Muted:
            color = loops_muted_color;
            break;
        case Playing:
            color = loops_playing_color;
            break;
    }
    canvas.at(pos.x + loops_top_left.x).at(pos.y + loops_top_left.y) = color;
    canvas.at(pos.x + loops_top_left.x + 1).at(pos.y + loops_top_left.y) = color;
    canvas.at(pos.x + loops_top_left.x + 1).at(pos.y + loops_top_left.y + 1) = color;
}

MainScreen::MainScreen():
    changed(false),
    song_duration(0),
    pattern_duration(0),
    loops({Off}),
    one_shots({Off}),
    pattern_seq_count(0),
    pattern_seq_index(0),
    load(0) {
}

MainScreen::~MainScreen() {    
}

bool MainScreen::draw(unicorn_hat_mini_canvas& canvas) {
    if (!changed) {
        return false;
    }
    changed = false;
    canvas.fill({0,0,0});
    draw_loops(canvas);
    return true;
}

void MainScreen::set_song_duration(std::chrono::seconds _song_duration) {
    song_duration = _song_duration;
    changed = true;
}

void MainScreen::set_pattern_duration(std::chrono::seconds _pattern_duration) {
    pattern_duration = _pattern_duration;
    changed = true;
}

void MainScreen::set_loop_playing(uint _number) {
    if (_number < loops.size()) {
        loops.at(_number) = Playing;
        changed = true;
    }
}

void MainScreen::set_loop_muted(uint _number) {
    if (_number < loops.size()) {
        loops.at(_number) = Muted;
        changed = true;
    }
}

void MainScreen::set_loop_off(uint _number) {
    if (_number < loops.size()) {
        loops.at(_number) = Off;
        changed = true;
    }
}

void MainScreen::set_one_shot_playing(uint _number) {
    if (_number < one_shots.size()) {
        one_shots.at(_number) = Playing;
        changed = true;
    }
}

void MainScreen::set_one_shot_muted(uint _number) {    
    if (_number < one_shots.size()) {
        one_shots.at(_number) = Muted;
        changed = true;
    }
}

void MainScreen::set_one_shot_off(uint _number) {    
    if (_number < one_shots.size()) {
        one_shots.at(_number) = Off;
        changed = true;
    }
}

void MainScreen::set_pattern_seq_count(uint _count) {    
    pattern_seq_count = _count;
    changed = true;
}

void MainScreen::set_pattern_seq_index(uint _index) {
    pattern_seq_index = _index;
    changed = true;
}

void MainScreen::set_load(float _level) {
    load = _level;
    changed = true;
}

Display::Display(std::chrono::milliseconds _refresh):
    refresh(_refresh),
    unicorn_hat_mini(),
    main_screen(),
    next_refresh() {

    // Test
    main_screen.set_loop_playing(0);
    main_screen.set_loop_muted(1);
    main_screen.set_loop_off(2);
    main_screen.set_loop_playing(3);
    main_screen.set_loop_muted(4);
    main_screen.set_loop_playing(5);

    main_screen.set_one_shot_muted(0);
    main_screen.set_one_shot_playing(1);
    main_screen.set_one_shot_muted(2);

    main_screen.set_pattern_seq_count(3);
    main_screen.set_pattern_seq_index(1);

    main_screen.set_song_duration(std::chrono::minutes(8));
    main_screen.set_pattern_duration(std::chrono::minutes(4));

    main_screen.set_load(0.4);
    tick();    
}

Display::~Display() {    
}

void Display::tick() {
    if (std::chrono::steady_clock::now() < next_refresh) {
        return;
    }
    next_refresh = std::chrono::steady_clock::now() + refresh;
    main_screen.draw(unicorn_hat_mini.get_canvas());
    unicorn_hat_mini.show();
}

MainScreen& Display::get_main_screen() {
    return main_screen;
}