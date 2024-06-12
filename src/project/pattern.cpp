#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "pattern.hpp"

PatternLoopSeq PatternLoop::get_or_default(size_t seq_index) {
    if (seq_index >= seq.size()) {
        SPDLOG_DEBUG("PRJKT pattern loop seq not found [track_number={},seq_index={}]", track_number, seq_index);
        return PatternLoopSeq();
    }
    SPDLOG_DEBUG("PRJKT pattern loop seq found [track_number={},seq_index={},muted={},saturation={}]", track_number, seq_index,
        seq.at(seq_index).muted, seq.at(seq_index).saturation);
    return seq.at(seq_index);
}

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
#if SPDLOG_ACTIVE_LEVEL < 2
            PatternLoop& inserted = result.emplace_back(PatternLoop(file.path(), track, {}));
            SPDLOG_DEBUG("PRJKT loop initialized [name={},track={}]", inserted.loop.filename().string(), inserted.track_number);
#else
            result.emplace_back(PatternLoop(file.path(), track, {}));
#endif
        }
    }
    return result;   
}

Pattern::Pattern(BankPattern root_bank_pattern, std::filesystem::path dir):
    bank_pattern(root_bank_pattern.get_program() + parse_pattern_offset(dir)),
    name(parse_name(dir)),
    pattern_number(0),
    loops(init_loops(dir)),
    mutes(),
    learned(false),
    seq_count(0) { 
    if (loops.empty()) {
        throw OstrostrojException(fmt::format("PRJKT no loops found! [dir={}]", dir.c_str()));
    }
    SPDLOG_DEBUG("PRJKT pattern initialized [bank_pattern={},name={},loops={}]", bank_pattern.get_pattern(), name, loops.size());
}

BankPattern Pattern::get_bank_pattern() {
    return bank_pattern;
}

void Pattern::set_number(size_t _number) {
    pattern_number = _number;
}

size_t Pattern::get_number() {
    return pattern_number;
}

std::string Pattern::get_name() {
    return name;
}

std::vector<PatternLoop>& Pattern::get_loops() {
    return loops;
}

std::array<std::vector<bool>, ModelCycles::MODEL_CYCLES_TRACK_COUNT>& Pattern::get_mutes() {
    return mutes;
}

void Pattern::set_learned() {
#ifndef NDEBUG
    SPDLOG_DEBUG("PRJKT pattern learned:");
    for (size_t i = 0; i < loops.size(); i++) {
        PatternLoop& loop = loops.at(i);
        std::string s = std::to_string(loop.track_number);
        for (size_t j = 0; j < loop.seq.size(); j++) {
            PatternLoopSeq& seq = loop.seq.at(j);
            if (seq.muted) {
                s.append("-");
            } else {
                s.append(std::to_string((int)(seq.saturation * 9.9) % 10));
            }
        }
        SPDLOG_DEBUG("L{}", s);
    }
    for (size_t i = 0; i < mutes.size(); i++) {
        std::string s;
        std::vector<bool>& m = mutes.at(i);
        for (size_t j = 0; j < m.size(); j++) {
            s.append(m.at(j) ? "-" : "|");
        }
        SPDLOG_DEBUG("T{}{}", i + 1, s);
    }
#endif
    learned = true;
}

bool Pattern::get_learned() {
    return learned;
}

void Pattern::unlearn() {
    for (PatternLoop& loop : loops) {
        loop.seq.clear();
    }
    mutes.fill({});
    seq_count = 0;
    learned = false;
    SPDLOG_DEBUG("PRJKT pattern unlearned");
}

void Pattern::update_seq_count() {
    seq_count = 0;    
    for (auto& mute : mutes) {
        size_t s = mute.size();
        if (s > seq_count) {
            seq_count = s;
        }
    }
    for (auto& loop : loops) {
        size_t s = loop.seq.size();
        if (s > seq_count) {
            seq_count = s;
        }
    }
    SPDLOG_DEBUG("PRJKT pattern update_seq_count[number={},seq_count={}]", pattern_number, seq_count);
}

size_t Pattern::get_seq_count() {
    return seq_count;
}