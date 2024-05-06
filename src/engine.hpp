#pragma once

#include <vector>
#include <thread>
#include <array>
#include "farbot/fifo.hpp"
#include "alsamidi.hpp"
#include "alsapcm.hpp"
#include "project.hpp"
#include "graph.hpp"

class Track {
    private:
        const int track_number;
        std::vector<std::reference_wrapper<PcmFifo>> channels;
        DynamicNode dynamic_node;
        TrackNode track_node;
        std::vector<float> next_frame;
        bool push_next_frame();
        void pop_next_frame(float sample);
    public:
        Track(int track_number, PcmFifo& channel);
        Track(int track_number, PcmFifo& left_channel, PcmFifo& right_channel);
        Track(int track_number, std::vector<std::reference_wrapper<PcmFifo>> channels);
        void set_node(std::unique_ptr<Node>&& node);
        void reset_node();
        void fill_output();
        void set_mute(bool mute);
        void start();
        void stop();
};

typedef farbot::fifo<std::function<void(void)>,
            farbot::fifo_options::concurrency::multiple,
            farbot::fifo_options::concurrency::single,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty> TrackTaskFifo;

class Engine {
    private:
        Project& project;
        AlsaMidiFifo& midi_fifo;
        std::atomic_flag& alsa_next_period_flag;

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
        Engine(Project& project, AlsaMidi& alsa_midi, AlsaPcm& alsa_pcm);
        virtual ~Engine();
        int get_loop_track_count() const;
};