#pragma once

#include "bank_pattern.hpp"
#include "clip.hpp"

static constexpr size_t PATTERN_MUTES = 6;

struct PatternLoopSeq {
    bool muted = true;
    // 0.0 - 1.0
    float saturation = 0.0; 
};

struct PatternLoop {
    std::filesystem::path loop;
    uint8_t track_number; // 1 - 6
    std::vector<PatternLoopSeq> seq;
    PatternLoopSeq get_or_default(size_t seq_index);
};

class Pattern {
    private:
        BankPattern bank_pattern;
        std::string name;
        size_t number;
        std::vector<PatternLoop> loops;
        std::array<std::vector<bool>, PATTERN_MUTES> mutes;
        bool learned;
        size_t seq_count;
        size_t parse_pattern_offset(std::filesystem::path dir);
        std::string parse_name(std::filesystem::path dir);
        std::vector<PatternLoop> init_loops(std::filesystem::path dir);
    public:
        Pattern(BankPattern root_bank_pattern, std::filesystem::path dir);
        virtual ~Pattern() = default;
        BankPattern get_bank_pattern();
        std::string get_name();
        void set_number(size_t _number);
        size_t get_number();
        std::vector<PatternLoop>& get_loops();
        std::array<std::vector<bool>, PATTERN_MUTES>& get_mutes();
        void set_learned();
        bool get_learned();
        void unlearn();
        void update_seq_count();
        size_t get_seq_count();
};