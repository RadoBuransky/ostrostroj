#pragma once

#include <vector>
#include <thread>
#include <array>
#include <jack/jack.h>
#include <libremidi/libremidi.hpp>
#include "farbot/fifo.hpp"
#include "project.hpp"
#include "soundcard.hpp"

class EngineTask;

typedef farbot::fifo<EngineTask,
            farbot::fifo_options::concurrency::multiple,
            farbot::fifo_options::concurrency::multiple,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
            8> EngineTaskFifo;

class EngineTask {
    public:
        void run(EngineTaskFifo &queue);
};

class Engine {
    private:
        const Project& project;
        SoundCard& soundCard;

        const std::vector<std::thread> threads;
        EngineTaskFifo tasks;
        std::atomic_bool interrupted;
        std::atomic_flag next_flag;
        std::mutex midi_processing_mutex;
        volatile bool midi_processed;

        std::vector<std::thread> create_threads();
        void run();
        void process_audio();
        void process_midi();

    public:
        Engine(const Project& project, SoundCard& soundCard);
        virtual ~Engine();

        void next();
};