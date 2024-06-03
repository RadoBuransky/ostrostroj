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
        const std::filesystem::path dir;
        BankPattern root_bank_pattern;
        std::vector<Pattern2> patterns;
        std::vector<SongOneShot> one_shots;
    public:
        Song(const std::filesystem::path _dir);
        virtual ~Song() = default;
        std::vector<Pattern2>& get_patterns();
        std::vector<SongOneShot>& get_one_shots();
};