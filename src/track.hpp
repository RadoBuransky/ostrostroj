#pragma once
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