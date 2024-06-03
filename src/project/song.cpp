#define SPDLOG_ACTIVE_LEVEL 1

#include "common.hpp"
#include "song.hpp"

static constexpr char DELIMITER = '_';

BankPattern Song::parse_root_bank_pattern(std::filesystem::path dir) {
    std::string s = dir.filename().string();
    return BankPattern(s.substr(0, s.find(DELIMITER)));
}

std::string Song::parse_name(std::filesystem::path dir) {
    std::string s = dir.filename().string();
    return s.substr(s.find(DELIMITER) + 1);
}

std::vector<Pattern> Song::init_patterns(std::filesystem::path dir) {   
    std::vector<Pattern> result;
    for (auto const& file : std::filesystem::directory_iterator(dir)) {
        if (file.is_directory()) {
            result.emplace_back(root_bank_pattern, file);
        }
    }
    std::sort(result.begin(), result.end(), [](Pattern& a, Pattern& b) {
        return a.get_bank_pattern().get_program() < b.get_bank_pattern().get_program();
    });
    return result; 
}

std::vector<SongOneShot> Song::init_one_shots(std::filesystem::path dir) {   
    std::vector<SongOneShot> result;
    for (auto const& file : std::filesystem::directory_iterator(dir)) {
        if (file.is_regular_file() && file.path().filename().string().starts_with('S')) {
            result.emplace_back(SongOneShot(file.path(), stoi(file.path().filename().string().substr(1, 1))));
        }
    }
    return result;   
}

Song::Song(std::filesystem::path dir):
    root_bank_pattern(parse_root_bank_pattern(dir)),
    name(parse_name(dir)),
    patterns(init_patterns(dir)),
    one_shots(init_one_shots(dir)) {
    SPDLOG_INFO("PRJKT song initialized [root_bank_pattern={},name={},patterns={},one_shots={}]", root_bank_pattern.get_pattern(),
        name, patterns.size(), one_shots.size());
}

BankPattern Song::get_root_bank_pattern() {
    return root_bank_pattern;    
}

std::string Song::get_name() {
    return name;
}

std::vector<Pattern>& Song::get_patterns() {
    return patterns;
}

std::vector<SongOneShot>& Song::get_one_shots() {    
    return one_shots;
}