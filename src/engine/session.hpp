#pragma once

#include "project.hpp"

class Session {
    private:
        Project2& project;
        std::map<std::filesystem::path, std::unique_ptr<FileClip>> clips;
        uint8_t active_song;
        uint8_t active_pattern;
        std::vector<uint8_t> pattern_play_counters;
        std::chrono::seconds song_duration;
        std::chrono::seconds pattern_duration;
        void load_clip(std::filesystem::path path, int expected_sample_rate, int loop_track_count);
        void load_all_clips(int expected_sample_rate, int loop_track_count);
    public:
        Session(Project2& _project, int expected_sample_rate, int loop_track_count);
        virtual ~Session() = default;
        void change_program(BankPattern pattern);
        Project2& get_project();
        Song& get_song();
        Pattern2& get_pattern();
        FileClip& get_clip(std::filesystem::path clip_path);
};