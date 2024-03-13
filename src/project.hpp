#pragma once

#include <vector>
#include <map>
#include "sample.hpp"

struct Program {
    private:
        int start_number;
        std::vector<LoopSample> loops;
        std::map<uint8_t, OneShotSample> one_shots;
};

class Project {
    private:
        std::vector<Program> programs;

    public:
        Project(std::filesystem::path dir);
        ~Project();
};

class ProjectStatus {
    private:
        Project project;
        int active_program_number;
        std::vector<SampleReader> loop_readers;
        std::vector<SampleReader> one_shot_readers;

    public:
        ProjectStatus(Project project);
        ~ProjectStatus();

        // TODO: Load all loops and one-shots
        void set_program(int program_number);
        void play_one_shot(uint8_t note);
};