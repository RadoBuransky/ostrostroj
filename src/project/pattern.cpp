#define SPDLOG_ACTIVE_LEVEL 2

#include "common.hpp"
#include "pattern.hpp"

size_t Pattern::parse_pattern_offset(std::filesystem::path dir) {
    return stoi(dir.filename().string().substr(0, 2));
}

std::string Pattern::parse_name(std::filesystem::path dir) {
    return dir.filename().string().substr(3);
}

std::vector<PatternLoop> Pattern::init_loops(std::filesystem::path dir) { 
    std::vector<PatternLoop> result;
    for (auto const& file : std::filesystem::directory_iterator(dir)) {
        if (file.is_regular_file() && file.path().filename().string().starts_with('L')) {
            uint8_t track = stoi(file.path().filename().string().substr(1, 1));
            PatternLoop& inserted = result.emplace_back(PatternLoop(file.path(), track));
            SPDLOG_DEBUG("PRJKT loop initialized [name={},track={}]", inserted.loop.filename().string(), inserted.track);
        }
    }
    return result;   
}

Pattern::Pattern(BankPattern root_bank_pattern, std::filesystem::path dir):
    bank_pattern(root_bank_pattern.get_program() + parse_pattern_offset(dir)),
    name(parse_name(dir)),
    number(0),
    loops(init_loops(dir)) { 
    SPDLOG_DEBUG("PRJKT pattern initialized [bank_pattern={},name={},loops={}]", bank_pattern.get_pattern(), name, loops.size());
}

BankPattern Pattern::get_bank_pattern() {
    return bank_pattern;
}

void Pattern::set_number(size_t _number) {
    number = _number;
}

size_t Pattern::get_number() {
    return number;
}

std::string Pattern::get_name() {
    return name;
}

std::vector<PatternLoop>& Pattern::get_loops() {
    return loops;
}