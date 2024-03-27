#pragma once

#include <vector>
#include <map>
#include "sample.hpp"

struct Program {
    private:
        int start_number;
        std::vector<LoopSample> loops;
        std::map<uint8_t, OneShotSample> one_shots;

        int program_start_number(const std::filesystem::path dir);
        std::vector<LoopSample> load_loops(const std::filesystem::path dir, const int expected_sample_rate);
        std::map<uint8_t, OneShotSample> load_one_shots(const std::filesystem::path dir, const int expected_sample_rate);
        std::vector<std::filesystem::path> wav_files(const std::filesystem::path dir);
        void check_sample_format(const std::filesystem::path path, const SF_INFO &format, const int expected_sample_rate,
                                 const int expected_channels) const;

    public:
        Program(const std::filesystem::path dir, const int expected_sample_rate);
        Program(Program&&);
        virtual ~Program();
        Program& operator=(Program&&);

        int get_start_number() const;
        std::vector<LoopSample> const & get_loops() const;
        std::map<uint8_t, OneShotSample> const & get_one_shots() const;
};

class Project {
    private:
        const std::vector<Program> programs;
        std::vector<Program> load_programs(const std::filesystem::path dir, const int expected_sample_rate);
    public:
        Project(const std::filesystem::path dir, const int expected_sample_rate);
        virtual ~Project();

        std::vector<Program> const & get_programs() const;
};