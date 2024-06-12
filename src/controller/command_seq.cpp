#include "common.hpp"
#include "command_seq.hpp"

static constexpr std::chrono::steady_clock::duration SEQ_TIMEOUT = std::chrono::seconds(3);