#pragma once

#include <vector>
#include <map>
#include "clip.hpp"
#include "song.hpp"

class Project;

class Program {
    private:
        friend Project;
        static constexpr int MONO_LOOP_TRACKS = 4;
        int start_number;
        std::vector<std::unique_ptr<LoopClip>> loops;
        std::map<uint8_t, std::unique_ptr<OneShotClip>> one_shots;

        int program_start_number(const std::filesystem::path dir);
        std::vector<std::unique_ptr<LoopClip>> load_loops(const std::filesystem::path dir);
        std::map<uint8_t, std::unique_ptr<OneShotClip>> load_one_shots(const std::filesystem::path dir);
        std::vector<std::filesystem::path> wav_files(const std::filesystem::path dir);
        void check_sample_format(const std::filesystem::path path, const SF_INFO &format,
                                 const int expected_channels) const;

    public:
        Program(std::filesystem::path dir);
        virtual ~Program() = default;
        int get_start_number() const;
        std::vector<std::reference_wrapper<LoopClip>> get_loops();
        std::map<uint8_t, std::reference_wrapper<OneShotClip>> get_one_shots();
};

class Project {
    private:
        std::vector<std::unique_ptr<Program>> programs;
        std::vector<std::unique_ptr<Program>> load_programs(const std::filesystem::path dir);
    public:
        Project(const std::filesystem::path dir);
        virtual ~Project() = default;
        void verify(const int expected_sample_rate, const int loop_track_count);
        Program& get_program(int program_number);
};

class Project2 {
    private:
        const std::filesystem::path dir;
        std::vector<Song> songs;
    public:
        Project2(const std::filesystem::path dir);
        virtual ~Project2();
        std::vector<Song>& get_songs();
};