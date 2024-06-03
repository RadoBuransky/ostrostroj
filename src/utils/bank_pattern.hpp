#pragma once

#include <string>

/**
 * A01 pattern = program 1
 * A02 pattern = program 2
 * ...
 * A16 pattern = program 16
 * B01 pattern = program 17
 * ...
 * H16 pattern = program 128
*/
class BankPattern {
    private:
        std::string pattern;
        uint8_t program;
    public:
        BankPattern(const std::string _pattern);
        BankPattern(const uint8_t _program);
        virtual ~BankPattern() = default;
        std::string get_pattern() const;
        uint8_t get_program() const;
};