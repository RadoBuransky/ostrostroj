#pragma once

#ifndef SPDLOG_ACTIVE_LEVEL
    #define SPDLOG_ACTIVE_LEVEL 2
#endif
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <vector>

class OstrostrojException : public std::runtime_error {
    public:
        OstrostrojException(const std::string &msg) : std::runtime_error{msg} {}
};

pthread_t create_rt_thread(std::string name, int sched_priority, void *(*start_routine) (void *), void* arg);