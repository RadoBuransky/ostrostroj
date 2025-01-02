#include "common.hpp"

#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>

#include "project.hpp"
#include "session.hpp"
#include "engine.hpp"

void Session::set_pattern(Song& song, Pattern& pattern) {
    active_song = song;
    active_pattern = pattern;
    update_display();
}

void Session::update_display() {
    MainScreen& main_screen = display.get_main_screen();

    main_screen.all_loops_off();
    for (PatternLoop& loop : active_pattern.get().get_loops()) {        
        main_screen.set_loop_state(loop.track_number - 1, false, 0.0);
    }

    main_screen.set_song_count(project.get_songs().size());
    main_screen.set_song_index(active_song.get().get_number() - 1);

    main_screen.set_pattern_count(active_song.get().get_patterns().size());    
    main_screen.set_pattern_index(active_pattern.get().get_number() - 1);

    main_screen.set_pattern_position(0);

    display.tick(true);
}

size_t Session::load_clip(std::filesystem::path path, int expected_channels) {
    if (clips.contains(path)) {
        return 0;
    }
    auto inserted = clips.emplace(path, std::make_unique<Clip>(path));
    if (inserted.second) {
        inserted.first->second->assert_format(sample_rate, expected_channels);
        if (sample_rate == 0) {
            sample_rate = inserted.first->second->get_info().samplerate;
        }
        return inserted.first->second->get_mem_size_bytes();
    }
    return 0;
}

size_t Session::load_all_clips(int loop_track_count) {
    size_t result = 0;
    for (Song& song: project.get_songs()) {
        for (Pattern& pattern: song.get_patterns()) {
            for (PatternLoop& pattern_loop: pattern.get_loops()) {    
                if (pattern_loop.track_number > loop_track_count) {
                    throw OstrostrojException(fmt::format("SESSN invalid loop track! [{}, {}]", pattern_loop.track_number, pattern_loop.loop.c_str()));
                }            
                result += load_clip(pattern_loop.loop, pattern_loop.track_number <= ENGINE_LOOP_MONO_TRACKS ? 1 : 2);
            }
        }
    }
    return result;
}

Session::Session(Project& _project, Display& _display, int loop_track_count):
    project(_project),
    display(_display),
    clips(),
    active_song(_project.get_songs().at(0)),
    active_pattern(_project.get_songs().at(0).get_patterns().at(0)),
    started_timestamp(std::chrono::steady_clock::time_point::min()),
    sample_rate(0) {
    mem_size_bytes = load_all_clips(loop_track_count);
    set_pattern(active_song, active_pattern);
    if (clips.empty()) {
        throw OstrostrojException("SESSN project contains no clips!");
    }
    SPDLOG_INFO("SESSN initialized [clips={},sample_rate={}Hz]", clips.size(), sample_rate);
}

bool Session::change_program(BankPattern target_pattern) {
    for (Song& song : project.get_songs() | std::views::reverse) {
        if (song.get_root_bank_pattern().get_program() <= target_pattern.get_program()) {
            for (Pattern& pattern : song.get_patterns() | std::views::reverse) {
                if (pattern.get_bank_pattern().get_program() <= target_pattern.get_program()) {
                    set_pattern(song, pattern);
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

Clip& Session::get_clip(std::filesystem::path clip_path) {
    return *clips.at(clip_path).get();
}

void Session::start() {    
    started_timestamp = std::chrono::steady_clock::now();
}

void Session::pause() {
    started_timestamp = std::chrono::steady_clock::time_point::min();
}

void Session::on_clock(float pattern_position) {
    MainScreen& main_screen = display.get_main_screen();
    main_screen.blink_clock();
    main_screen.set_pattern_position(pattern_position);
    display.tick(false);
}

size_t Session::get_mem_size_bytes() const {
    return mem_size_bytes;
}

snd_pcm_uframes_t Session::get_sample_rate() const {
    return sample_rate;
}