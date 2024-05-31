#pragma once

#include "bank_pattern.hpp"
#include "clip.hpp"

class Pattern {
    private:
        const std::filesystem::path dir;
        BankPattern bank_pattern;
        std::vector<LoopClip2> loops;
    public:
        Pattern(const std::filesystem::path dir);
        virtual ~Pattern();
        std::vector<LoopClip2>& get_loops();
};