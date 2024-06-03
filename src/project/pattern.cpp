#include "common.hpp"
#include "pattern.hpp"

Pattern2::Pattern2(const std::filesystem::path _dir):
    dir(_dir),
    bank_pattern(_dir.filename()) { // TODO: Parse dir
}

BankPattern Pattern2::get_bank_pattern() {
    return bank_pattern;
}

std::vector<PatternLoop>& Pattern2::get_loops() {
    return loops;
}