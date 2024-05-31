#pragma once

#include "bank_pattern.hpp"
#include "clip.hpp"

class Pattern2 {
    private:
        const std::filesystem::path dir;
        BankPattern bank_pattern;
        std::vector<LoopClip2> loops;
    public:
        Pattern2(const std::filesystem::path dir);
        virtual ~Pattern2();
        std::vector<LoopClip2>& get_loops();
};