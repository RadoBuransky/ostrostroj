#include "common.hpp"
#include "bank_pattern.hpp"

BankPattern::BankPattern(const std::string _pattern):
    pattern(_pattern), // TODO:
    program() {

}

BankPattern::BankPattern(const uint8_t _program):
    pattern(), // TODO:
    program(_program) {
}

std::string BankPattern::get_pattern() const {
    return pattern;
}

uint8_t BankPattern::get_program() const {
    return program;
}