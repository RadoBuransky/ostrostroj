#include "common.hpp"
#include "project.hpp"
#include "session.hpp"

void Session::load_clip(std::filesystem::path path, int expected_sample_rate, int loop_track_count) {
    if (clips.contains(path)) {
        return;
    }
    auto inserted = clips.emplace(path, std::make_unique<FileClip>(path));
    if (inserted.second) {
        SF_INFO& format = inserted.first->second->get_info();
        // TODO: assert 
        // TODO: assert format.channels;
        // TODO: assert format.samplerate;
    }
}

void Session::load_all_clips(int expected_sample_rate, int loop_track_count) {
    for (Song& song: project.get_songs()) {
        for (SongOneShot& song_one_shot: song.get_one_shots()) {
            load_clip(song_one_shot.one_shot, expected_sample_rate, loop_track_count);
        }
        for (Pattern2& pattern: song.get_patterns()) {
            for (PatternLoop& pattern_loop: pattern.get_loops()) {                
                load_clip(pattern_loop.loop, expected_sample_rate, loop_track_count);
            }
        }
    }
}

Session::Session(Project2& _project, int expected_sample_rate, int loop_track_count):
    project(_project),
    clips() {
    load_all_clips(expected_sample_rate, loop_track_count);
}

void Session::change_program(BankPattern pattern) {
    // TODO:
}

Project2& Session::get_project() {
    return project;
}

Song& Session::get_song() {
    return project.get_songs().at(active_song);
}

Pattern2& Session::get_pattern() {
    return get_song().get_patterns().at(active_pattern);
}

FileClip& Session::get_clip(std::filesystem::path clip_path) {
    return *clips.at(clip_path).get();
}