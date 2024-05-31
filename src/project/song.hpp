#pragma once

#include "bank_pattern.hpp"
#include "pattern.hpp"
#include "clip.hpp"

class Song {
    private:
        const std::filesystem::path dir;
        BankPattern root_bank_pattern;
        std::vector<Pattern> patterns;
        std::vector<OneShotClip2> one_shots;
    public:
        Song(const std::filesystem::path dir);
        virtual ~Song();
        std::vector<Pattern>& get_patterns();
        std::vector<OneShotClip2>& get_one_shots();
};