#pragma once

#include <algorithm>
#include <alsa/asoundlib.h>
#include <alsa/pcm.h>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <errno.h>
#include <farbot/fifo.hpp>
#include <filesystem>
#include <functional>
#include <gpiod.h>
#include <iostream>
#include <linux/spi/spidev.h>
#include <map>
#include <mutex>
#include <random>
#include <ranges>
#include <samplerate.h>
#include <signal.h>
#include <spidev_lib++.h>
#include <sndfile.hh>
#include <stdexcept>
#include <string>
#include <sys/reboot.h>
#include <unistd.h>
#include <vector>

static constexpr int PCM_OUT_CHANNELS = 12;

static constexpr int ENGINE_LOOP_TRACKS = 6;
static constexpr int ENGINE_LOOP_MONO_TRACKS = 4;
static_assert(ENGINE_LOOP_MONO_TRACKS + (ENGINE_LOOP_TRACKS - ENGINE_LOOP_MONO_TRACKS) * 2 + 2 < PCM_OUT_CHANNELS);

static constexpr uint8_t L1_PARAM = 111; // CC #111
static constexpr uint8_t ONE_SHOT_PARAM = 119; // CC #119

class OstrostrojException : public std::runtime_error {
    public:
        OstrostrojException(const std::string &msg) : std::runtime_error{msg} {}
};

pthread_t create_rt_thread(std::string name, int sched_priority, void *(*start_routine) (void *), void* arg);