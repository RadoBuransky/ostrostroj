#pragma once

#include <vector>
#include <map>
#include "clip.hpp"

class Program {
    private:
        int start_number;
        std::vector<LoopClip> loops;
        std::map<uint8_t, OneShotClip> one_shots;

        int program_start_number(const std::filesystem::path dir);
        std::vector<LoopClip> load_loops(const std::filesystem::path dir);
        std::map<uint8_t, OneShotClip> load_one_shots(const std::filesystem::path dir);
        std::vector<std::filesystem::path> wav_files(const std::filesystem::path dir);
        void check_sample_format(const std::filesystem::path path, const SF_INFO &format,
                                 const int expected_channels) const;

    public:
        Program(const std::filesystem::path dir);
        int get_start_number() const;
        std::vector<LoopClip>& get_loops();
        std::map<uint8_t, OneShotClip>& get_one_shots();
};

class Project {
    private:
        std::vector<Program> programs;
        std::vector<Program> load_programs(const std::filesystem::path dir);
    public:
        Project(const std::filesystem::path dir);
        void assert_sample_rate(const int expected_sample_rate);
        Program& get_program(int program_number);
};