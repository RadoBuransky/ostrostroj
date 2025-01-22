#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "main_screen.hpp"

constexpr RGB palette_playing = palette_red;
constexpr RGB palette_muted = palette_blue;

MainScreen::MainScreen(Canvas& _canvas):
    Screen(_canvas),
    changed(true),
    loops(),
    song_count(0),
    active_song_index(0),
    selected_song_index(0),
    pattern_count(0),
    active_pattern_index(0),
    selected_pattern_index(0),
    pattern_fade_mix(-1.0),
    pattern_position(0.0),
    clock_on(false),
    playing(false) {
    loops.fill({true, 0.0, false});
}

bool MainScreen::draw() {
    if (!changed) {
        return false;
    }
    changed = false;
    canvas.clear();
    
    // MIDI clock
    canvas.point(canvas.get_last_col(), canvas.get_last_row(), (clock_on) ? (playing ? palette_red : palette_white) : palette_off);

    // Pattern position
    canvas.line(0, pattern_position, canvas.get_last_row(), palette_white);

    if (song_count > 0 && pattern_count > 0) {
        // Active song/pattern
        canvas.progress(3, song_count, active_song_index, palette_white);
        canvas.progress(4, pattern_count, -1, palette_white);

        // Selected song/pattern
        if (selected_song_index == active_song_index) {
            canvas.point(canvas.get_progress_point_col(pattern_count, active_pattern_index), 4, palette_white);
        } else {
            canvas.point(canvas.get_progress_point_col(song_count, selected_song_index), 3, palette_red);
        }
        if (selected_pattern_index != active_pattern_index || selected_song_index != active_song_index) {
            canvas.point(canvas.get_progress_point_col(pattern_count, selected_pattern_index), 4, palette_red);
        }
    }

    if (pattern_fade_mix >= 0.0) {
        canvas.point(std::round(pattern_fade_mix * canvas.get_cols()), 5, palette_magenta);
    }
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

void MainScreen::set_active_song_index(uint _song_index) {
    active_song_index = _song_index;
    changed = true;
}

void MainScreen::set_selected_song_index(uint _song_index) {
    selected_song_index = _song_index;
    changed = true;
}

void MainScreen::set_pattern_count(uint _pattern_count) {
    pattern_count = _pattern_count;
    changed = true;
}

void MainScreen::set_active_pattern_index(uint _pattern_index) {
    active_pattern_index = _pattern_index;
    changed = true;
}

void MainScreen::set_selected_pattern_index(uint _pattern_index) {
    selected_pattern_index = _pattern_index;
    changed = true;
}

void MainScreen::set_pattern_fade(float _mix) {
    pattern_fade_mix =_mix;
    changed = true;
}

void MainScreen::set_pattern_position(float _pattern_position) {
    size_t new_pattern_position = std::max(0, (int) std::round((float) canvas.get_last_col() * std::min(_pattern_position, 1.0f)));
    if (new_pattern_position == pattern_position) {
        return;
    }
    pattern_position = new_pattern_position;
    changed = true;
}

void MainScreen::set_clock(bool _on) {
    clock_on = _on;
    changed = true;
}

void MainScreen::set_playing(bool _playing) {
    playing = _playing;
    changed = true;
}