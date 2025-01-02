#pragma once

#include "bank_pattern.hpp"
#include "clip.hpp"

struct PatternLoop {
    std::filesystem::path loop;
    uint8_t track_number; // 1 - 6
};

class Pattern {
    private:
        BankPattern bank_pattern;
        std::string name;
        size_t pattern_number;
        std::vector<PatternLoop> loops;
        size_t parse_pattern_offset(std::filesystem::path dir);
        std::string parse_name(std::filesystem::path dir);
        std::vector<PatternLoop> init_loops(std::filesystem::path dir);
    public:
        Pattern(BankPattern root_bank_pattern, std::filesystem::path dir);
        virtual ~Pattern() = default;
        BankPattern get_bank_pattern();
        std::string get_name();
        void set_number(size_t _number);
        size_t get_number();
        std::vector<PatternLoop>& get_loops();
};