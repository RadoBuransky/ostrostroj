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
        const bool no_xrun;
        std::unique_ptr<InterleavedFifo> fifo;
        std::atomic_bool stop;
        DynamicNode dynamic_node;
        TrackNode track_node;
        std::mutex m;    
        std::condition_variable cv;
        std::thread worker_thread;
        void run();
    public:
        Track(int _track_number, int _channels, std::chrono::milliseconds _period_time, snd_pcm_uframes_t _period_size, bool _no_xrun);
        virtual ~Track();
        InterleavedFifo& get_fifo() const;
        int get_channels() const;
        void reset_node();
        void set_node(std::unique_ptr<Node>&& node);
};