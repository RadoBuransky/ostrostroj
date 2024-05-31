#include "common.hpp"
#include "project.hpp"
#include "session.hpp"

void Session::load_clip(std::filesystem::path path, int expected_sample_rate, int loop_track_count) {
    if (clips.contains(path)) {
        return;
    }
    clips.emplace(path, std::make_unique<FileClipBlock>(nullptr, 0, 0)); // TODO: ...
}

void Session::load_all_clips(int expected_sample_rate, int loop_track_count) {
    for (Song& song: project.get_songs()) {
        for (OneShotClip2& one_shot_clip: song.get_one_shots()) {
            load_clip(one_shot_clip.get_path(), expected_sample_rate, loop_track_count);
        }
        for (Pattern2& pattern: song.get_patterns()) {
            for (LoopClip2& loop_clip: pattern.get_loops()) {
                load_clip(loop_clip.get_path(), expected_sample_rate, loop_track_count);
            }
        }
    }
}

Session::Session(Project2& _project, int expected_sample_rate, int loop_track_count):
    project(_project),
    clips() {
    load_all_clips(expected_sample_rate, loop_track_count);
}