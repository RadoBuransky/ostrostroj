#pragma once

#include <vector>
#include <thread>
#include <array>
#include "farbot/fifo.hpp"

class AudioOutputTask {
    public:
        AudioOutputTask();
        virtual ~AudioOutputTask();

        bool run() const;
};

typedef farbot::fifo<AudioOutputTask,
            farbot::fifo_options::concurrency::multiple,
            farbot::fifo_options::concurrency::multiple,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
            8> AudioOutputTaskFifo;

class Engine {
    private:
        const std::vector<std::thread> threads;
        AudioOutputTaskFifo queue;
        std::atomic_bool interrupted;

        std::vector<std::thread> create_threads();
        void run();

    public:
        Engine();
        virtual ~Engine();
};