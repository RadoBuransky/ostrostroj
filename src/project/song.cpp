#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
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
    for (size_t i = 0; i < result.size(); i++) {
        result.at(i).set_number(i + 1);
    }
    return result; 
}

Song::Song(std::filesystem::path dir):
    root_bank_pattern(parse_root_bank_pattern(dir)),
    name(parse_name(dir)),
    number(0),
    patterns(init_patterns(dir)) {
    if (patterns.empty()) {
        throw OstrostrojException(fmt::format("PRJKT no patterns found! [dir={}]", dir.c_str()));
    }
    SPDLOG_INFO("PRJKT song initialized [root_bank_pattern={},name={},patterns={}]", root_bank_pattern.get_pattern(), name, patterns.size());
}

BankPattern Song::get_root_bank_pattern() {
    return root_bank_pattern;    
}

std::string Song::get_name() {
    return name;
}

void Song::set_number(size_t _number) {
    number = _number;
}

size_t Song::get_number() {
    return number;
}

std::vector<Pattern>& Song::get_patterns() {
    return patterns;
}