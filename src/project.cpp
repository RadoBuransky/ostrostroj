#include <algorithm>
#include <ranges>
#include "spdlog/spdlog.h"
#include "project.hpp"
#include "common.hpp"

Program::Program(const std::filesystem::path dir):
    start_number(program_start_number(dir)),
    loops(load_loops(dir)),
    one_shots(load_one_shots(dir)) {
    spdlog::info(std::format("Program loaded {} [{} loops, {} one shots]", dir.string(), loops.size(), one_shots.size()));
}

int Program::program_start_number(const std::filesystem::path dir) {
    return std::stoi(dir.filename().string().substr(1, 2));
}

std::vector<LoopClip> Program::load_loops(const std::filesystem::path dir) {
    spdlog::debug(std::format("Loading loops {} ", dir.string()));
    auto result = std::vector<LoopClip>();
    for (auto const& wav_file : wav_files(dir)) {
        if (wav_file.filename().string().starts_with("L")) {
            auto loop = LoopClip(wav_file);
            const int expected_channels = (loop.get_track() < 5) ? 1 : 2;
            check_sample_format(wav_file, loop.get_info(), expected_channels);
            result.push_back(std::move(loop));
        }
    }
    return result;
}

std::map<uint8_t, OneShotClip> Program::load_one_shots(const std::filesystem::path dir) {
    spdlog::debug(std::format("Loading one shots {} ", dir.string()));
    std::map<uint8_t, OneShotClip> result = std::map<uint8_t, OneShotClip>();
    for (auto const& wav_file : wav_files(dir)) {
        if (wav_file.filename().string().starts_with("S")) {
            auto one_shot_sample = OneShotClip(wav_file);
            check_sample_format(wav_file, one_shot_sample.get_info(), 2);
            result.insert(std::make_pair(one_shot_sample.get_note(), std::move(one_shot_sample)));
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

void Program::check_sample_format(const std::filesystem::path wav_file, const SF_INFO &format, const int expected_channels) const {
    if ((format.format & SF_FORMAT_WAV) == 0) {
        throw OstrostrojException(std::format("WAV file expected! [{:x}, {}]", format.format, wav_file.string()));
    }
    if ((format.format & SF_FORMAT_FLOAT) == 0) {
        throw OstrostrojException(std::format("32-bit float expected! [{:x}, {}]", format.format, wav_file.string()));
    }
    if (format.channels != expected_channels) {
        throw OstrostrojException(std::format("{} channels expected! [{}, {}]", expected_channels, format.channels, wav_file.string()));
    }
}

int Program::get_start_number() const {
    return start_number;
}

std::vector<LoopClip>& Program::get_loops() {
    return loops;
}

std::map<uint8_t, OneShotClip>& Program::get_one_shots() {
    return one_shots;
}

Project::Project(const std::filesystem::path dir):
    programs(load_programs(dir)) {
}

std::vector<Program> Project::load_programs(const std::filesystem::path dir) {
    spdlog::info(std::format("Loading project from {}", dir.string()));
    auto result = std::vector<Program>();
    for (auto const& program_dir : std::filesystem::directory_iterator(dir)) {
        if (program_dir.is_directory() && program_dir.path().filename().string().starts_with("P")) {
            result.push_back(Program(program_dir));
        }
    }
    std::sort(result.begin(), result.end(), [](const Program& a, const Program& b) {
        return a.get_start_number() < b.get_start_number();
    });
    return result;
}

void Project::verify(const int expected_sample_rate, const int loop_track_count) {
    for (Program& program : programs) {
        for (const LoopClip& loop : program.get_loops()) {
            loop.assert_sample_rate(expected_sample_rate);
            if (loop.get_track() < 0 || loop.get_track() >= loop_track_count) {
                throw OstrostrojException(std::format("Invalid loop track! [{}, {}]", loop.get_track(), loop.get_path().c_str()));
            }
        }
        for (const auto& [note, one_shot] : program.get_one_shots()) {
            one_shot.assert_sample_rate(expected_sample_rate);
        }
    }
}

Program& Project::get_program(int program_number) {
    for (Program& program : programs | std::views::reverse) {
        if (program.get_start_number() <= program_number) {
            return program;
        }
    }
    return programs.at(0);
}