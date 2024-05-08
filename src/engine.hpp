#pragma once

#include "alsamidi.hpp"
#include "alsapcm.hpp"
#include "project.hpp"

class Engine {
    public:
        Engine(Project& project, AlsaMidi& alsa_midi, AlsaPcm& _alsa_pcm);
        virtual ~Engine();
        int get_loop_track_count() const;
        void pcm_callback();
        void midi_callback();
};

/*
#include <vector>
#include <thread>
#include <array>
#include "farbot/fifo.hpp"
#include "alsamidi.hpp"
#include "alsapcm.hpp"
#include "project.hpp"
#include "graph.hpp"


typedef farbot::fifo<std::function<void(void)>,
            farbot::fifo_options::concurrency::multiple,
            farbot::fifo_options::concurrency::multiple,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty> TrackTaskFifo;

class Engine {
    private:
        Project& project;
        AlsaPcm& alsa_pcm;
        AlsaMidiFifo& midi_fifo;
        std::atomic_flag next_flag;

        std::array<std::unique_ptr<Track>, 6> loop_tracks;
        Track one_shots_track;

        std::vector<std::thread> threads;
        TrackTaskFifo tasks;

        std::atomic_bool interrupted;
        std::mutex midi_processing_mutex;
        std::atomic_bool midi_processed;
        std::atomic_int program_number;

        void create_threads();

        void run();
        void create_tasks();
        void create_track_task(Track& track);
        void run_tasks();
        
        void process_midi();
        void midi_start();
        void midi_stop();
        void midi_continue();
        void play_one_shot(uint8_t note);
        void set_program(int program_number);

    public:
        Engine(Project& project, AlsaMidi& alsa_midi, AlsaPcm& _alsa_pcm);
        virtual ~Engine();
        int get_loop_track_count() const;
        void pcm_callback();
        void midi_callback();
};
*/