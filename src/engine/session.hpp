#pragma once

#include "project.hpp"

class Session {
    private:
        Project2& project;
        std::map<std::filesystem::path, std::unique_ptr<FileClipBlock>> clips;
        uint8_t active_song;
        uint8_t active_pattern;
        std::vector<uint8_t> pattern_play_counters;
        std::chrono::seconds song_duration;
        std::chrono::seconds pattern_duration;

    public:
        Session(Project2& project);
        virtual ~Session();
        Project2& get_project();

        // TODO: Load project clips (is "path" ok as a map key?)
        // TODO: Setters, getters, start/stop duration counters
};