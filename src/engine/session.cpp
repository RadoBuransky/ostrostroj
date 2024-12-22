#include "common.hpp"

#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>

#include "project.hpp"
#include "session.hpp"
#include "engine.hpp"

void Session::set_pattern(Song& song, Pattern& pattern, bool running) {
    if (!running || (song.get_number() != active_song.get().get_number())) {
        pattern_play_counters.clear();
    }    
    inc_pattern_play_counters(song, pattern);
    if (!running) {
        song.unlearn();
    }
    active_song = song;
    active_pattern = pattern;
    update_display();
}

void Session::update_display() {
    MainScreen& main_screen = display.get_main_screen();

    song_duration = std::chrono::steady_clock::duration::zero();
    main_screen.set_song_duration(song_duration);

    pattern_duration = std::chrono::steady_clock::duration::zero();
    main_screen.set_pattern_duration(pattern_duration);

    main_screen.all_loops_off();
    for (PatternLoop& loop : active_pattern.get().get_loops()) {        
        main_screen.set_loop_state(loop.track_number - 1, loop.get_or_default(get_seq_index()));
    }

    main_screen.all_one_shots_off();
    for (SongOneShot& one_shot : active_song.get().get_one_shots()) {
        main_screen.set_one_shot_state(one_shot.number - 1, Muted);
    }

    main_screen.set_song_count(project.get_songs().size());    
    main_screen.set_song_index(active_song.get().get_number() - 1);

    main_screen.set_pattern_count(active_song.get().get_patterns().size());    
    main_screen.set_pattern_index(active_pattern.get().get_number() - 1);

    main_screen.set_pattern_seq_count(active_pattern.get().get_seq_count());
    main_screen.set_pattern_seq_index(get_seq_index());

    display.tick(true);
}

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

void Session::inc_pattern_play_counters(Song& song, Pattern& pattern) {
    auto it = pattern_play_counters.find(pattern.get_number() - 1);
    if (it == pattern_play_counters.end()) {
        pattern_play_counters.emplace(pattern.get_number() - 1, 1);
        return;
    }
    pattern_play_counters.at(it->first)++;
}

size_t Session::get_seq_index() {
    return pattern_play_counters.at(active_pattern.get().get_number() - 1) - 1;
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
        for (SongOneShot& song_one_shot: song.get_one_shots()) {
            result += load_clip(song_one_shot.one_shot, 2);
        }
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
    pattern_play_counters(),
    song_duration(0),
    pattern_duration(0),
    started_timestamp(std::chrono::steady_clock::time_point::min()),
    sample_rate(0) {
    mem_size_bytes = load_all_clips(loop_track_count);
    set_pattern(active_song, active_pattern, false);
    if (clips.empty()) {
        throw OstrostrojException("SESSN project contains no clips!");
    }
    SPDLOG_INFO("SESSN initialized [clips={},sample_rate={}Hz]", clips.size(), sample_rate);
}

bool Session::change_program(BankPattern target_pattern, bool running) {
    for (Song& song : project.get_songs() | std::views::reverse) {
        if (song.get_root_bank_pattern().get_program() <= target_pattern.get_program()) {
            for (Pattern& pattern : song.get_patterns() | std::views::reverse) {
                if (pattern.get_bank_pattern().get_program() <= target_pattern.get_program()) {
                    set_pattern(song, pattern, running);
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
        
PatternLoopSeq Session::get_current_loop_seq(uint8_t track_number) {
    for (PatternLoop& loop : active_pattern.get().get_loops()) {
        if (loop.track_number == track_number) {
            return loop.get_or_default(get_seq_index());
        }
    }
    SPDLOG_DEBUG("SESSN current loop seq not found [track_number={}]", track_number);
    return PatternLoopSeq();
}

bool Session::get_current_mute(uint8_t mc_track_number) {
    auto& mutes = active_pattern.get().get_mutes().at(mc_track_number - 1);
    size_t seq_index = get_seq_index();
    if (seq_index < mutes.size()) {
        return mutes.at(seq_index);
    }
    // Muted by default
    return true;
}

Clip& Session::get_clip(std::filesystem::path clip_path) {
    return *clips.at(clip_path).get();
}

std::optional<std::reference_wrapper<Clip>> Session::get_current_one_shot_clip() {
    size_t seq_index = get_seq_index();
    std::vector<uint8_t> one_shot_numbers = active_pattern.get().get_one_shots();
    if (seq_index >= one_shot_numbers.size()) {
        return std::nullopt;
    }
    uint8_t one_shot_number = one_shot_numbers.at(seq_index);
    if (one_shot_number == 0) {
        return std::nullopt;
    }
    std::optional<std::reference_wrapper<SongOneShot>> one_shot_maybe = active_song.get().get_one_shot(one_shot_number);
    if (!one_shot_maybe.has_value()) {
        return std::nullopt;
    }
    return *clips.at(one_shot_maybe.value().get().one_shot);
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

void Session::step_learned() {
    update_display();
}

size_t Session::get_mem_size_bytes() const {
    return mem_size_bytes;
}

snd_pcm_uframes_t Session::get_sample_rate() const {
    return sample_rate;
}