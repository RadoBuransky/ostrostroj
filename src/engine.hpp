#pragma once

#include <vector>
#include <thread>
#include <array>
#include <jack/jack.h>
#include <libremidi/libremidi.hpp>
#include "farbot/fifo.hpp"
#include "project.hpp"
#include "soundcard.hpp"
#include "graph.hpp"

class Track {
    private:
        std::vector<std::reference_wrapper<AudioFifo>> channels;
        DynamicNode dynamic_node;
        MuteNode mute_node;
        TransportNode transport_node;
        volatile bool mute;

    public:
        Track(AudioFifo& channel);
        Track(AudioFifo& left_channel, AudioFifo& right_channel);
        Track(std::vector<std::reference_wrapper<AudioFifo>> channels);
        void set_node(std::unique_ptr<Node> node);
        void reset_node();

        void fill_output();
        void set_mute(bool mute);
        void start();
        void stop();
};

typedef farbot::fifo<std::function<void(void)>,
            farbot::fifo_options::concurrency::multiple,
            farbot::fifo_options::concurrency::multiple,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
            8> TrackTaskFifo;

class Engine {
    private:
        Project& project;
        SoundCard& soundCard;

        std::array<std::unique_ptr<Track>, 6> loop_tracks;
        Track one_shots_track;

        const std::vector<std::thread> threads;
        TrackTaskFifo tasks;

        std::atomic_bool interrupted;
        std::atomic_flag next_flag;
        std::mutex midi_processing_mutex;
        volatile bool midi_processed;

        std::vector<std::thread> create_threads();

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
        Engine(Project& project, SoundCard& soundCard);
        virtual ~Engine();

        void next();
};