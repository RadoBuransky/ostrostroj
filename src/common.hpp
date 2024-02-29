#pragma once

#include <stdexcept>
#include <string>

class OstrostrojException : public std::runtime_error {
    public:
        OstrostrojException(const std::string &msg) : std::runtime_error{msg} {}
};