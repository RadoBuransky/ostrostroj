#pragma once

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <format>
#include <string>
#include <vector>

class OstrostrojException : public std::runtime_error {
    public:
        OstrostrojException(const std::string &msg) : std::runtime_error{msg} {}
};
