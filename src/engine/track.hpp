#pragma once

#include "farbot/fifo.hpp"
#include "alsapcm.hpp"
#include "clipplayer.hpp"
#include "warp.hpp"
#include "saturation.hpp"

typedef farbot::fifo<PcmSample_s24_3le,
            farbot::fifo_options::concurrency::single,
            farbot::fifo_options::concurrency::single,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty> InterleavedFifo;

class Track {
    private:
        const int track_number;
        const int channels;
        snd_pcm_uframes_t period_size;
        uint8_t periods;
        const bool loop;
        std::unique_ptr<InterleavedFifo> fifo;
        std::vector<std::unique_ptr<ClipPlayer>> clip_players; 
        PcmSample_s24_3le sample;
        bool sample_pending;
        Warp warp;
        Saturation saturation;
        bool pop(float& _sample);
    public:
        Track(int _track_number, int _channels, snd_pcm_uframes_t _period_size, uint8_t _periods, bool _loop);
        virtual ~Track();        
        void run();
        InterleavedFifo& get_fifo() const;
        int get_track_number() const;
        int get_channels() const;
        void add_clip(Clip& clip, snd_pcm_uframes_t latency, bool predelay);
        void clear(bool drop);
        void set_saturation(float _saturation);
        float get_saturation();
        float get_position(std::filesystem::path& clip_path);
};