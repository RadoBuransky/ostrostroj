#pragma once

#ifndef SPDLOG_ACTIVE_LEVEL
    #define SPDLOG_ACTIVE_LEVEL 2
#endif
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <vector>

static constexpr int PCM_OUT_CHANNELS = 12;

static constexpr int ENGINE_LOOP_TRACKS = 6;
static constexpr int ENGINE_LOOP_MONO_TRACKS = 4;
static_assert(ENGINE_LOOP_MONO_TRACKS + (ENGINE_LOOP_TRACKS - ENGINE_LOOP_MONO_TRACKS) * 2 + 2 < PCM_OUT_CHANNELS);

static constexpr uint8_t L1_PARAM = 111; // CC #111

class OstrostrojException : public std::runtime_error {
    public:
        OstrostrojException(const std::string &msg) : std::runtime_error{msg} {}
};

pthread_t create_rt_thread(std::string name, int sched_priority, void *(*start_routine) (void *), void* arg);