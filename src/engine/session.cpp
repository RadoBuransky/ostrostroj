#include "common.hpp"
#include <algorithm>
#include <ranges>
#include "project.hpp"
#include "session.hpp"
        
static constexpr int MONO_LOOP_TRACKS = 4;

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
                if (pattern_loop.track >= loop_track_count) {
                    throw OstrostrojException(fmt::format("PRJKT invalid loop track! [{}, {}]", pattern_loop.track, pattern_loop.loop.c_str()));
                }            
                load_clip(pattern_loop.loop, expected_sample_rate, pattern_loop.track < MONO_LOOP_TRACKS ? 1 : 2);
            }
        }
    }
}

Session::Session(Project& _project, int expected_sample_rate, int loop_track_count):
    project(_project),
    clips(),
    active_song(_project.get_songs().at(0)),
    active_pattern(_project.get_songs().at(0).get_patterns().at(0)) {
    load_all_clips(expected_sample_rate, loop_track_count);
}

void Session::change_program(BankPattern target_pattern) {
    for (Song& song : project.get_songs() | std::views::reverse) {
        if (song.get_root_bank_pattern().get_program() <= target_pattern.get_program()) {
            active_song = song;
            for (Pattern pattern : song.get_patterns() | std::views::reverse) {
                if (pattern.get_bank_pattern().get_program() <= target_pattern.get_program()) {
                    active_pattern = pattern;
                    SPDLOG_INFO("SESSN Program changed. [song={},pattern={}]", active_song.get().get_name(), active_pattern.get().get_name());
                    return;
                }
            }
            SPDLOG_ERROR("SESSN No pattern found! [song={},program={}]", song.get_name(), target_pattern.get_program());
        }
    }
    SPDLOG_ERROR("SESSN No song found! [program={}]", target_pattern.get_program());
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