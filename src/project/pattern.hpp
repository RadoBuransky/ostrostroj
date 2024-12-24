#pragma once

#include "bank_pattern.hpp"
#include "clip.hpp"
#include "model_cycles.hpp"

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
        std::vector<uint8_t> one_shots; // One-shot number 1, 2, 3, ... 0 means no one-shot to be played.
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
        std::vector<uint8_t>& get_one_shots();
};