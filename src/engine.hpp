#pragma once

#include <vector>
#include <thread>
#include <array>
#include <jack/jack.h>
#include <libremidi/libremidi.hpp>
#include "farbot/fifo.hpp"
#include "project.hpp"
#include "soundcard.hpp"

typedef std::function<void(void)> EngineTask;

typedef farbot::fifo<EngineTask,
            farbot::fifo_options::concurrency::multiple,
            farbot::fifo_options::concurrency::multiple,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
            8> EngineTaskFifo;

class Track {
    protected:
        volatile bool mute;
        volatile std::unique_ptr<SampleReader> sample_reader;

    public:
        virtual void fill_output() = 0;

        void set_mute(bool mute);
        bool get_mute() const;

        void set_sample(Sample sample);
};

class MonoTrack : Track {
    private:
        AudioFifo& output;
    public:
        MonoTrack(AudioFifo& output);
        virtual void fill_output();
};

class StereoTrack : Track {
    private:
        AudioFifo& left_output;
        AudioFifo& right_output;

    public:
        StereoTrack(AudioFifo& left_output, AudioFifo& right_output);
        virtual void fill_output();
};

// TODO:
// - engine has a fixed list of tracks
// - one task per track
// - each task fills output.

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
        void process_midi();
        void create_tasks();
        void run_tasks();

        void play_one_shot(uint8_t note);

        void set_active_program(int program_number);

    public:
        Engine(const Project& project, SoundCard& soundCard);
        virtual ~Engine();

        void next();
};