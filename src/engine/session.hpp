#pragma once

#include "project.hpp"
#include "display.hpp"
#include "clipplayer.hpp"

class Session {
    private:
        Project& project;
        Display& display;
        std::map<std::filesystem::path, std::unique_ptr<Clip>> clips;
        std::reference_wrapper<Song> active_song;
        std::reference_wrapper<Pattern> active_pattern;
        // TODO: Which clip player do we use to show current position (the last one?)
        // TODO: Who handles transition process? Maybe a standalone class?
        std::chrono::steady_clock::time_point started_timestamp;
        size_t mem_size_bytes;
        snd_pcm_uframes_t sample_rate;
        void set_pattern(Song& song, Pattern& pattern);
        void update_display();
        size_t load_clip(std::filesystem::path path, int expected_channels);
        size_t load_all_clips(int loop_track_count);
    public:
        Session(Project& _project, Display& _display, int loop_track_count);
        virtual ~Session() = default;
        bool change_program(BankPattern target_pattern);
        Project& get_project();
        Song& get_song();
        Pattern& get_pattern();
        Clip& get_clip(std::filesystem::path clip_path);
        void start();
        void pause();
        void on_clock(float pattern_position);
        size_t get_mem_size_bytes() const;
        snd_pcm_uframes_t get_sample_rate() const;
};