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
        MidiFifo midi_fifo;
        // FIFO for each audio output port
        std::vector<std::unique_ptr<AudioFifo>> audio_output_fifos;

        const std::vector<std::thread> threads;
        EngineTaskFifo tasks;
        std::atomic_bool interrupted;
        std::atomic_flag next_flag;
        std::mutex midi_processing_mutex;
        volatile bool midi_processed;

        std::vector<std::unique_ptr<AudioFifo>> create_audio_output_fifos(int audio_outputs, jack_nframes_t audio_output_size);
        std::vector<std::thread> create_threads();
        void run();
        void process_audio();
        void process_midi();

    public:
        Engine(int audio_outputs, jack_nframes_t audio_output_size);
        virtual ~Engine();

        MidiFifo& get_midi_fifo();
        AudioFifo& get_audio_output_fifo(int port);
        void next();
};