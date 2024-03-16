#include "spdlog/spdlog.h"
#include "project.hpp"

Program::Program(const std::filesystem::path dir):
    start_number(program_start_number(dir)),
    loops(load_loops(dir)),
    one_shots(load_one_shots(dir)) {
    spdlog::info(std::format("Program loaded {} [{} loops, {} one shots]", dir.string(), loops.size(), one_shots.size()));
}

Program::~Program() {    
}

int Program::program_start_number(const std::filesystem::path dir) {
    return std::stoi(dir.string().substr(1, 2));
}

std::vector<LoopSample> Program::load_loops(const std::filesystem::path dir) {
    auto result = std::vector<LoopSample>();
    for (auto const& wav_file : wav_files(dir)) {
        if (wav_file.string().starts_with("L")) {
            result.push_back(LoopSample(wav_file));
        }
    }
    return result;
}

std::map<uint8_t, OneShotSample> Program::load_one_shots(const std::filesystem::path dir) {
    auto result = std::map<uint8_t, OneShotSample>();
    for (auto const& wav_file : wav_files(dir)) {
        if (wav_file.string().starts_with("S")) {
            auto one_shot_sample = OneShotSample(wav_file);
            result.insert({one_shot_sample.get_note(), one_shot_sample});
        }
    }
    return result;
}

std::vector<std::filesystem::path> Program::wav_files(const std::filesystem::path dir) {
    auto result = std::vector<std::filesystem::path>();
    for (auto const& file : std::filesystem::directory_iterator(dir)) {
        auto const& path = file.path();
        if (file.is_regular_file() && path.string().ends_with(".wav")) {
            result.push_back(file);
        }
    }
    return result;
}

Project::Project(const std::filesystem::path dir):
    programs(loadPrograms(dir)) {
}

Project::~Project() {
}

std::vector<Program> Project::loadPrograms(const std::filesystem::path dir) {
    spdlog::info(std::format("Loading project from {}", dir.string()));
    auto result = std::vector<Program>();
    for (auto const& program_dir : std::filesystem::directory_iterator(dir)) {
        if (program_dir.is_directory() && program_dir.path().string().starts_with("P")) {
            result.push_back(Program(program_dir));
        }
    }
    return result;
}