#pragma once

#define SPDLOG_ACTIVE_LEVEL 1
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <vector>

class OstrostrojException : public std::runtime_error {
    public:
        OstrostrojException(const std::string &msg) : std::runtime_error{msg} {}
};
