#pragma once

#include <string>

class BankPattern {
    private:
        const std::string pattern;
        const uint8_t program;
    public:
        BankPattern(const std::string _pattern);
        BankPattern(const uint8_t _program);
        virtual ~BankPattern() = default;
        std::string get_pattern() const;
        uint8_t get_program() const;
};