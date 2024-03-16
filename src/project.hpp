#pragma once

#include <vector>
#include <map>
#include "sample.hpp"

struct Program {
    private:
        const int start_number;
        const std::vector<LoopSample> loops;
        const std::map<uint8_t, OneShotSample> one_shots;

        int program_start_number(const std::filesystem::path dir);
        std::vector<LoopSample> load_loops(const std::filesystem::path dir, const int expected_sample_rate);
        std::map<uint8_t, OneShotSample> load_one_shots(const std::filesystem::path dir, const int expected_sample_rate);
        std::vector<std::filesystem::path> wav_files(const std::filesystem::path dir);
        void check_sample_format(const std::filesystem::path path, const SF_INFO &format, const int expected_sample_rate,
                                 const int expected_channels) const;

    public:
        Program(const std::filesystem::path dir, const int expected_sample_rate);
        virtual ~Program();

        std::vector<LoopSample> get_loops() const;
        std::map<uint8_t, OneShotSample> get_one_shots() const;
};

class Project {
    private:
        const std::vector<Program> programs;
        std::vector<Program> load_programs(const std::filesystem::path dir, const int expected_sample_rate);
    public:
        Project(const std::filesystem::path dir, const int expected_sample_rate);
        virtual ~Project();
};

class ProjectStatus {
    private:
        const Project project;
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