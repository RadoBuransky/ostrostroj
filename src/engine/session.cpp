#define SPDLOG_ACTIVE_LEVEL 2

#include "common.hpp"
#include <algorithm>
#include <ranges>
#include "project.hpp"
#include "session.hpp"
#include "engine.hpp"

void Session::update_durations() {
    if (started_timestamp != std::chrono::steady_clock::time_point::min()) {
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        std::chrono::steady_clock::duration d = now - started_timestamp;
        if (d > std::chrono::seconds(1)) {
            song_duration += d;
            pattern_duration += d;
            started_timestamp = now;

            MainScreen& main_screen = display.get_main_screen();
            main_screen.set_song_duration(song_duration);
            main_screen.set_pattern_duration(pattern_duration);
        }
    }
}

void Session::load_clip(std::filesystem::path path, int expected_sample_rate, int expected_channels) {
    if (clips.contains(path)) {
        return;
    }
    auto inserted = clips.emplace(path, std::make_unique<FileClip>(path));
    if (inserted.second) {
        inserted.first->second->assert_format(expected_sample_rate, expected_channels);
    }
}

void Session::load_all_clips(int expected_sample_rate, int loop_track_count) {
    for (Song& song: project.get_songs()) {
        for (SongOneShot& song_one_shot: song.get_one_shots()) {
            load_clip(song_one_shot.one_shot, expected_sample_rate, 2);
        }
        for (Pattern& pattern: song.get_patterns()) {
            for (PatternLoop& pattern_loop: pattern.get_loops()) {    
                if (pattern_loop.track > loop_track_count) {
                    throw OstrostrojException(fmt::format("SESSN invalid loop track! [{}, {}]", pattern_loop.track, pattern_loop.loop.c_str()));
                }            
                load_clip(pattern_loop.loop, expected_sample_rate, pattern_loop.track <= ENGINE_LOOP_MONO_TRACKS ? 1 : 2);
            }
        }
    }
}

Session::Session(Project& _project, Display& _display, int expected_sample_rate, int loop_track_count):
    project(_project),
    display(_display),
    clips(),
    active_song(_project.get_songs().at(0)),
    active_pattern(_project.get_songs().at(0).get_patterns().at(0)),
    pattern_play_counters(),
    song_duration(0),
    pattern_duration(0),
    started_timestamp(std::chrono::steady_clock::time_point::min()) {
    load_all_clips(expected_sample_rate, loop_track_count);

    MainScreen& main_screen = display.get_main_screen();
    main_screen.set_song_count(project.get_songs().size());
    main_screen.set_song_index(0);
    main_screen.set_pattern_count(active_song.get().get_patterns().size());
    main_screen.set_pattern_index(0);
}

bool Session::change_program(BankPattern target_pattern) {
    for (Song& song : project.get_songs() | std::views::reverse) {
        if (song.get_root_bank_pattern().get_program() <= target_pattern.get_program()) {
            for (Pattern& pattern : song.get_patterns() | std::views::reverse) {
                if (pattern.get_bank_pattern().get_program() <= target_pattern.get_program()) {
                    active_song = song;
                    active_pattern = pattern;
                    display.get_main_screen().set_song_index(active_song.get().get_number() - 1);
                    display.get_main_screen().set_pattern_index(active_pattern.get().get_number() - 1);
                    SPDLOG_INFO("SESSN program changed [song={},pattern={}]", active_song.get().get_name(), active_pattern.get().get_name());
                    return true;
                }
            }
            SPDLOG_ERROR("SESSN No pattern found! [song={},program={}]", song.get_name(), target_pattern.get_program());
        }
    }
    SPDLOG_ERROR("SESSN No song found! [program={}]", target_pattern.get_program());
    return false;
}

Project& Session::get_project() {
    return project;
}

Song& Session::get_song() {
    return active_song;
}

Pattern& Session::get_pattern() {
    return active_pattern;
}

FileClip& Session::get_clip(std::filesystem::path clip_path) {
    return *clips.at(clip_path).get();
}

void Session::start() {    
    started_timestamp = std::chrono::steady_clock::now();
}

void Session::pause() {
    update_durations();
    started_timestamp = std::chrono::steady_clock::time_point::min();
}

void Session::draw() {
    update_durations();
    display.tick(false);
}