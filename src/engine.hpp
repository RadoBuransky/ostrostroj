#pragma once

#include <vector>
#include <thread>
#include <array>
#include <jack/jack.h>
#include <libremidi/libremidi.hpp>
#include "farbot/fifo.hpp"

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

typedef farbot::fifo<jack_default_audio_sample_t,
            farbot::fifo_options::concurrency::single,
            farbot::fifo_options::concurrency::single,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
            1> AudioFifo;

typedef farbot::fifo<libremidi::message,
            farbot::fifo_options::concurrency::single,
            farbot::fifo_options::concurrency::single,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
            1> MidiFifo;

class Engine {
    private:
        std::unique_ptr<MidiFifo> midi_fifo;

        const std::vector<std::thread> threads;
        EngineTaskFifo queue;
        std::atomic_bool interrupted;
        std::atomic_flag next_flag;
        std::mutex midi_processing_mutex;
        volatile bool midi_processed;

        std::vector<std::thread> create_threads();
        void run();

    public:
        Engine();
        virtual ~Engine();

        MidiFifo& get_midi_fifo();
        void next();
};