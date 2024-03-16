#include "spdlog/spdlog.h"
#include "project.hpp"

Program::Program(const std::filesystem::path dir):
    start_number(0),
    loops(std::vector<LoopSample>()),
    one_shots(std::map<uint8_t, OneShotSample>()) {
    spdlog::info(std::format("Program loaded {}", dir.string()));
}

Program::~Program() {    
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
        result.push_back(Program(program_dir));
    }
    return result;
}