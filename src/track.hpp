#pragma once

#include <chrono>
#include <alsa/asoundlib.h>
#include "farbot/fifo.hpp"
#include "alsapcm.hpp"
#include "graph.hpp"

typedef farbot::fifo<PcmSample_s24_3le,
            farbot::fifo_options::concurrency::single,
            farbot::fifo_options::concurrency::single,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty> InterleavedFifo;

class Track {
    private:
        const int track_number;
        const int channels;
        const std::chrono::milliseconds period_time;
        std::unique_ptr<InterleavedFifo> fifo;
        std::atomic_bool stop;
        DynamicNode dynamic_node;
        TrackNode track_node;
        std::thread worker_thread;
        void run();
        inline bool node_to_fifo(std::array<float, PCM_OUT_CHANNELS>& in_frame, PcmSample_s24_3le& out_frame, InterleavedFifo& fifo_ref);
    public:
        Track(int _track_number, int _channels, std::chrono::milliseconds _period_time, snd_pcm_uframes_t _period_size);
        virtual ~Track();
};

/*
#include <vector>
#include "alsapcm.hpp"
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
*/