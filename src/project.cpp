#include "spdlog/spdlog.h"
#include "project.hpp"
#include "common.hpp"

Program::Program(const std::filesystem::path dir, const int expected_sample_rate):
    start_number(program_start_number(dir)),
    loops(load_loops(dir, expected_sample_rate)),
    one_shots(load_one_shots(dir, expected_sample_rate)) {
    spdlog::info(std::format("Program loaded {} [{} loops, {} one shots]", dir.string(), loops.size(), one_shots.size()));
}

Program::~Program() {    
}

int Program::program_start_number(const std::filesystem::path dir) {
    return std::stoi(dir.string().substr(1, 2));
}

std::vector<LoopSample> Program::load_loops(const std::filesystem::path dir, const int expected_sample_rate) {
    auto result = std::vector<LoopSample>();
    for (auto const& wav_file : wav_files(dir)) {
        if (wav_file.string().starts_with("L")) {
            auto loop = LoopSample(wav_file);
            const int expected_channels = (loop.get_track() < 5) ? 1 : 2;
            check_sample_format(wav_file, loop.get_info(), expected_sample_rate, expected_channels);
            result.push_back(loop);
        }
    }
    return result;
}

std::map<uint8_t, OneShotSample> Program::load_one_shots(const std::filesystem::path dir, const int expected_sample_rate) {
    auto result = std::map<uint8_t, OneShotSample>();
    for (auto const& wav_file : wav_files(dir)) {
        if (wav_file.string().starts_with("S")) {
            auto one_shot_sample = OneShotSample(wav_file);
            check_sample_format(wav_file, one_shot_sample.get_info(), expected_sample_rate, 2);
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

void Program::check_sample_format(const std::filesystem::path wav_file, const SF_INFO &format, const int expected_sample_rate,
                                  const int expected_channels) const {
    if ((format.format & SF_FORMAT_WAV) == 0) {
        throw OstrostrojException(std::format("WAV file expected! [{:x}, {}]", format.format, wav_file.string()));
    }
    if (expected_sample_rate != format.samplerate) {
        throw OstrostrojException(std::format("Invalid sample rate! [{}Hz, {}]", format.samplerate, wav_file.string()));
    }
    if ((format.format & SF_FORMAT_FLOAT) == 0) {
        throw OstrostrojException(std::format("32-bit float expected! [{:x}, {}]", format.format, wav_file.string()));
    }
    if (format.channels != expected_channels) {
        throw OstrostrojException(std::format("{} channels expected! [{}, {}]", expected_channels, format.channels, wav_file.string()));
    }
}

std::vector<LoopSample> Program::get_loops() const {
    return loops;
}

std::map<uint8_t, OneShotSample> Program::get_one_shots() const {
    return one_shots;
}

Project::Project(const std::filesystem::path dir, const int expected_sample_rate):
    programs(load_programs(dir, expected_sample_rate)) {
}

Project::~Project() {
}

std::vector<Program> Project::load_programs(const std::filesystem::path dir, const int expected_sample_rate) {
    spdlog::info(std::format("Loading project from {}", dir.string()));
    auto result = std::vector<Program>();
    for (auto const& program_dir : std::filesystem::directory_iterator(dir)) {
        if (program_dir.is_directory() && program_dir.path().string().starts_with("P")) {
            result.push_back(Program(program_dir, expected_sample_rate));
        }
    }
    return result;
}