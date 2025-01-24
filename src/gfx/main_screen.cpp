#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "main_screen.hpp"

void MainScreen::draw_clock() {
    if (playback_duration == std::chrono::steady_clock::duration::zero()) {
        return;
    }
    int hours = std::chrono::duration_cast<std::chrono::hours>(playback_duration).count();
    int minutes_in_hour = std::chrono::duration_cast<std::chrono::minutes>(playback_duration).count() % 60;
    int seconds = std::chrono::duration_cast<std::chrono::seconds>(playback_duration).count();
    RGB quarter_color = hours == 0 ? palette_white : palette_red;
    if (seconds > 0) {
        canvas.point(1, 0, quarter_color);
    }
    if (minutes_in_hour >= 15) {
        canvas.point(1, 1, quarter_color);
    }
    if (minutes_in_hour >= 30) {
        canvas.point(0, 1, quarter_color);
    }
    if (minutes_in_hour >= 45) {
        canvas.point(0, 0, quarter_color);
    }
    float quarter_fraction = (float)(seconds % 900) / 900.0;
    uint8_t quarter_cols = std::ceil(quarter_fraction * (canvas.get_cols() - 2));
    canvas.line(2, 1 + quarter_cols, 0, palette_blue);
    if (quarter_cols >= 5) {
        canvas.point(6, 0, palette_red);
    }
    if (quarter_cols >= 10) {
        canvas.point(11, 0, palette_red);
    }
    if (quarter_cols >= 15) {
        canvas.point(16, 0, palette_red);
    }
}

void MainScreen::draw_loop(uint8_t col, uint8_t row, LoopState loop_state) {
    RGB color;
    if (loop_state.muted) {
        color = palette_blue;
    } else if (loop_state.saturation == 0.0) {
        color = palette_white;
    } else {        
        color = palette_red * std::max(0.05f, std::pow(loop_state.saturation * 0.5f, 2.0f));
    }
    canvas.point(col, row, color);
}

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
    playing(false),
    playback_duration(std::chrono::steady_clock::duration::zero()) {
    loops.fill({true, 0.0, false});
}

bool MainScreen::draw() {
    if (!changed) {
        return false;
    }
    changed = false;
    canvas.clear();

    // Pattern position
    canvas.line(0, pattern_position, canvas.get_last_row(), palette_white);
    
    // MIDI clock
    if (clock_on) {
        canvas.point(canvas.get_last_col(), canvas.get_last_row(), playing ? palette_red : palette_white);
    }

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

    // Pattern fading
    if (pattern_fade_mix >= 0.0) {
        canvas.point(std::round(pattern_fade_mix * canvas.get_cols()), 5, palette_magenta);
    }

    // Clock
    draw_clock();

    // Loops
    draw_loop(canvas.get_cols() - 4, 1, loops.at(0));
    draw_loop(canvas.get_cols() - 3, 1, loops.at(1));
    draw_loop(canvas.get_cols() - 2, 1, loops.at(2));
    draw_loop(canvas.get_cols() - 1, 1, loops.at(3));
    draw_loop(canvas.get_cols() - 4, 2, loops.at(4));
    draw_loop(canvas.get_cols() - 3, 2, loops.at(5));

    return true;
}

void MainScreen::set_loop_muted(size_t loop_index, bool muted) {
    if (loop_index >= loops.size()) {
        return;
    }
    LoopState& loopState = loops.at(loop_index);
    loopState.muted = muted;
    changed = true;
}

void MainScreen::mute_all_loops() {
    for (LoopState& loop : loops) {
        loop.muted = true;
    }
}

void MainScreen::set_loop_saturation(size_t loop_index, float saturation) {
    if (loop_index >= loops.size()) {
        return;
    }
    LoopState& loopState = loops.at(loop_index);
    loopState.saturation = saturation;
    changed = true;
}

void MainScreen::set_loop_grabbed(size_t loop_index, bool grabbed) {
    loops.at(loop_index).grabbed = grabbed;
    changed = true;
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

void MainScreen::set_playback_duration(std::chrono::steady_clock::duration _playback_duration) {
    playback_duration = _playback_duration;
    changed = true;
}