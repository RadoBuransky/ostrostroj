#pragma once

#include "bank_pattern.hpp"
#include "pattern.hpp"
#include "clip.hpp"

struct SongOneShot {
    std::filesystem::path one_shot;
    uint8_t number;
};

class Song {
    private:
        BankPattern root_bank_pattern;
        std::string name;
        std::vector<Pattern> patterns;
        std::vector<SongOneShot> one_shots;
        BankPattern parse_root_bank_pattern(std::filesystem::path dir);
        std::string parse_name(std::filesystem::path dir);
        std::vector<Pattern> init_patterns(std::filesystem::path dir);
        std::vector<SongOneShot> init_one_shots(std::filesystem::path dir);
    public:
        Song(std::filesystem::path dir);
        virtual ~Song() = default;
        BankPattern get_root_bank_pattern();
        std::string get_name();
        std::vector<Pattern>& get_patterns();
        std::vector<SongOneShot>& get_one_shots();
};