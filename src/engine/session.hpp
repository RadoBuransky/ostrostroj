#pragma once

#include "project.hpp"
#include "display.hpp"

class Session {
    private:
        Project& project;
        Display& display;
        std::map<std::filesystem::path, std::unique_ptr<Clip>> clips;
        std::reference_wrapper<Song> active_song;
        std::reference_wrapper<Pattern> active_pattern;
        std::map<uint8_t, uint8_t> pattern_play_counters; // Pattern index -> Number of times it was started to played
        std::chrono::steady_clock::duration song_duration;
        std::chrono::steady_clock::duration pattern_duration;
        std::chrono::steady_clock::time_point started_timestamp;
        void set_pattern(Song& song, Pattern& pattern, bool running);
        void update_durations();
        void inc_pattern_play_counters(Song& song, Pattern& pattern);
        void load_clip(std::filesystem::path path, int expected_sample_rate, int expected_channels);
        void load_all_clips(int expected_sample_rate, int loop_track_count);
    public:
        Session(Project& _project, Display& _display, int expected_sample_rate, int loop_track_count);
        virtual ~Session() = default;
        bool change_program(BankPattern target_pattern, bool running);
        Project& get_project();
        Song& get_song();
        Pattern& get_pattern();
        PatternLoopSeq get_current_loop_seq(uint8_t track_number);
        Clip& get_clip(std::filesystem::path clip_path);
        void start();
        void pause();
        void draw();
        void step_learned();
};