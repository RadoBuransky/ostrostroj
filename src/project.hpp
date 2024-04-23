#pragma once

#include <vector>
#include <map>
#include "clip.hpp"

class Program {
    private:
        static constexpr int MONO_LOOP_TRACKS = 4;
        int start_number;
        std::vector<std::unique_ptr<LoopClip>> loops;
        std::map<uint8_t, std::unique_ptr<OneShotClip>> one_shots;

        int program_start_number(const std::filesystem::path dir);
        std::vector<LoopClip> load_loops(const std::filesystem::path dir);
        std::map<uint8_t, OneShotClip> load_one_shots(const std::filesystem::path dir);
        std::vector<std::filesystem::path> wav_files(const std::filesystem::path dir);
        void check_sample_format(const std::filesystem::path path, const SF_INFO &format,
                                 const int expected_channels) const;

    public:
        Program(const std::filesystem::path dir);
        int get_start_number() const;
        std::vector<std::reference_wrapper<LoopClip>> get_loops();
        std::map<uint8_t, std::reference_wrapper<OneShotClip>> get_one_shots();
};

class Project {
    private:
        std::vector<std::unique_ptr<Program>> programs;
        void load_programs(const std::filesystem::path dir);
    public:
        Project(const std::filesystem::path dir);
        void verify(const int expected_sample_rate, const int loop_track_count);
        Program& get_program(int program_number);
};