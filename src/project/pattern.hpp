#pragma once

#include "bank_pattern.hpp"
#include "clip.hpp"

struct PatternLoop {
    std::filesystem::path loop;
    uint8_t track; // 1 - 6
};

class Pattern {
    private:
        BankPattern bank_pattern;
        std::string name;
        std::vector<PatternLoop> loops;
        size_t parse_pattern_offset(std::filesystem::path dir);
        std::string parse_name(std::filesystem::path dir);
        std::vector<PatternLoop> init_loops(std::filesystem::path dir);
    public:
        Pattern(BankPattern root_bank_pattern, std::filesystem::path dir);
        virtual ~Pattern() = default;
        BankPattern get_bank_pattern();
        std::string get_name();
        std::vector<PatternLoop>& get_loops();
};