#pragma once

#include "project.hpp"

class Session {
    private:
        Project& project;
        std::map<std::filesystem::path, std::unique_ptr<FileClip>> clips;
        std::reference_wrapper<Song> active_song;
        std::reference_wrapper<Pattern> active_pattern;
        std::vector<uint8_t> pattern_play_counters;
        std::chrono::seconds song_duration;
        std::chrono::seconds pattern_duration;
        void load_clip(std::filesystem::path path, int expected_sample_rate, int expected_channels);
        void load_all_clips(int expected_sample_rate, int loop_track_count);
    public:
        Session(Project& _project, int expected_sample_rate, int loop_track_count);
        virtual ~Session() = default;
        void change_program(BankPattern target_pattern);
        Project& get_project();
        Song& get_song();
        Pattern& get_pattern();
        FileClip& get_clip(std::filesystem::path clip_path);
};