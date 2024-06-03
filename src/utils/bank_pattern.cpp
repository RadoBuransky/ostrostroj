#include "common.hpp"
#include "bank_pattern.hpp"

BankPattern::BankPattern(const std::string _pattern):
    pattern(_pattern),
    program(((std::toupper(_pattern.at(0)) - 'A') * 16) + stoi(_pattern.substr(1)) - 1) {
}

BankPattern::BankPattern(const uint8_t _program):
    pattern(fmt::format("{:c}{:02}", (char)('A' + (_program / 16)), _program % 16)),
    program(_program) {
}

std::string BankPattern::get_pattern() const {
    return pattern;
}

uint8_t BankPattern::get_program() const {
    return program;
}