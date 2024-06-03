#define SPDLOG_ACTIVE_LEVEL 1

#include "common.hpp"
#include <algorithm>
#include <ranges>
#include "project.hpp"

static constexpr char DELIMITER = '_';

uint8_t Project::parse_number(std::filesystem::path dir) {
    std::string s = dir.filename().string();
    return std::stoi(s.substr(0, s.find(DELIMITER)));
}

std::string Project::parse_name(std::filesystem::path dir) {
    std::string s = dir.filename().string();
    return s.substr(s.find(DELIMITER) + 1);
}

std::vector<Song> Project::init_songs(std::filesystem::path dir) {
    std::vector<Song> result;
    for (auto const& file : std::filesystem::directory_iterator(dir)) {
        if (file.is_directory()) {
            result.emplace_back(file);
        }
    }
    std::sort(result.begin(), result.end(), [](Song& a, Song& b) {
        return a.get_root_bank_pattern().get_program() < b.get_root_bank_pattern().get_program();
    });
    return result;
}

Project::Project(std::filesystem::path dir):
    number(parse_number(dir)),
    name(parse_name(dir)),
    songs(init_songs(dir)) {
    SPDLOG_INFO("PRJKT project initialized [number={},name={},songs={}]", number, name, songs.size());
}

uint8_t Project::get_number() {
    return number;
}

std::string Project::get_name() {
    return name;
}

std::vector<Song>& Project::get_songs() {
    return songs;
}