#include "common.hpp"
#include "display.hpp"

constexpr RGB pattern_duration_color = {0, 0, RGB_01};
constexpr RGB song_duration_color = {0, RGB_10, 0};

constexpr Point loops_top_left = {9, 0};
constexpr RGB loops_off_color = {0, 0, 0};
constexpr RGB loops_muted_color = {0, 0, RGB_01};
constexpr RGB loops_playing_color = {RGB_MAX, 0, 0};

constexpr RGB song_count_color = {0, RGB_10, 0};
constexpr RGB song_index_color = {0, 0, RGB_01};

void MainScreen::draw_song_and_pattern_duration(unicorn_hat_mini_canvas& canvas) {
    uint song_width = std::min((uint)std::chrono::duration_cast<std::chrono::minutes>(song_duration).count(), (uint)9);
    uint pattern_width = std::min((uint)std::chrono::duration_cast<std::chrono::minutes>(pattern_duration).count(), (uint)song_width);
    for (uint i = 0; i < song_width; i++) {
        canvas.at(i).at(0) = (i < pattern_width) ? pattern_duration_color : song_duration_color;
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
    RGB color = track_state_color(track_state);
    canvas.at(pos.x + loops_top_left.x).at(pos.y + loops_top_left.y) = color;
    canvas.at(pos.x + loops_top_left.x + 1).at(pos.y + loops_top_left.y) = color;
    canvas.at(pos.x + loops_top_left.x + 1).at(pos.y + loops_top_left.y + 1) = color;
}

void MainScreen::draw_songs(unicorn_hat_mini_canvas& canvas) {
    uint count = std::min(song_count, (uint)2*9);
    for (uint i = 0; i < count; i++) {
        canvas.at(i % 9).at(1 + (i / 9)) = i < song_index ? song_index_color : song_count_color;
    }
}

void MainScreen::draw_one_shots(unicorn_hat_mini_canvas& canvas) {
    for (uint i = 0; i < one_shots.size(); i++) {
        uint x = (i / 2) * 2;
        uint y = 4 + ((i % 2) * 2);
        RGB color;
        switch (one_shots[i]) {
            case Muted:
                color = loops_playing_color;
                break;
            case Playing:
                color = loops_muted_color;
                break;
            default:
                color = loops_off_color;
                break;
        }
        canvas.at(x).at(y) = color;
    }
}

RGB MainScreen::track_state_color(TrackState track_state) {
    switch (track_state) {
        case Muted:
            return loops_muted_color;
        case Playing:
            return loops_playing_color;
        default:
            return loops_off_color;
    }
}

void MainScreen::draw_patterns(unicorn_hat_mini_canvas& canvas) {
    uint count = std::min(pattern_count, (uint)8);
    for (uint i = 0; i < count; i++) {
        canvas.at(UNICORN_HAT_MINI_COLS - count + i).at(5) = (i == pattern_index) ? pattern_duration_color : song_duration_color;
    }
}

void MainScreen::draw_pattern_seq(unicorn_hat_mini_canvas& canvas) {
    uint count = std::min(pattern_seq_count - pattern_seq_index, (uint)8);
    for (uint i = 0; i < count; i++) {
        canvas.at(UNICORN_HAT_MINI_COLS - count + i).at(6) = loops_playing_color;
    }    
}

MainScreen::MainScreen():
    changed(false),
    song_duration(0),
    pattern_duration(0),
    loops(),
    one_shots(),
    pattern_seq_count(0),
    pattern_seq_index(0) {
    loops.fill(Off);
    one_shots.fill(Off);

    // Test
    song_duration = std::chrono::minutes(7);
    pattern_duration = std::chrono::minutes(3);
    loops[0] = Muted;
    loops[1] = Playing;
    loops[2] = Muted;
    loops[3] = Muted;
    loops[4] = Muted;
    one_shots[0] = Muted;
    one_shots[1] = Muted;
    one_shots[2] = Playing;
    one_shots[3] = Muted;
    one_shots[4] = Muted;
    song_count = 15;
    song_index = 3;
    pattern_count = 6;
    pattern_index = 2;
    pattern_seq_count = 3;
    pattern_seq_index = 0;
}

MainScreen::~MainScreen() {    
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
    draw_pattern_seq(canvas);
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

    main_screen.set_song_duration(std::chrono::minutes(8));
    main_screen.set_pattern_duration(std::chrono::minutes(4));

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