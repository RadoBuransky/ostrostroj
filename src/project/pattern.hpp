#pragma once

#include "bank_pattern.hpp"
#include "clip.hpp"

struct PatternLoop {
    std::filesystem::path loop;
    uint8_t track; // 1 - 6
};

class Pattern2 {
    private:
        const std::filesystem::path dir;
        BankPattern bank_pattern;
        std::vector<PatternLoop> loops;
    public:
        Pattern2(const std::filesystem::path _dir);
        virtual ~Pattern2() = default;
        BankPattern get_bank_pattern();
        std::vector<PatternLoop>& get_loops();
};