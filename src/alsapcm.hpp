#pragma once
#include <alsa/pcm.h>
#include "farbot/fifo.hpp"

typedef farbot::fifo<float,
            farbot::fifo_options::concurrency::single,
            farbot::fifo_options::concurrency::single,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty> PcmFifo;

class AlsaPcm {
    private:
        snd_pcm_t* pcm_out;
        snd_pcm_uframes_t buffer_size;
        snd_pcm_uframes_t period_size;
        std::atomic_bool stop;
        const std::vector<std::unique_ptr<PcmFifo>> channel_fifos;
        pthread_t pcm_thread;
        std::function<void(void)> callback;
        friend void* run_pcm(void* context);
        void float_to_s24_3le(float sample, unsigned char* buffer);
        int set_hwparams(snd_pcm_t* handle, snd_pcm_hw_params_t* params);
        int set_swparams(snd_pcm_t* handle, snd_pcm_sw_params_t* swparams);
        snd_pcm_t* open_pcm_out(const std::string& pcm_out_name);
        std::vector<std::unique_ptr<PcmFifo>> create_channel_fifos();
    public:
        AlsaPcm();
        virtual ~AlsaPcm();
        void start(std::function<void(void)> _callback);
        snd_pcm_uframes_t get_sample_rate() const;
        int get_channels() const;
        PcmFifo& get_channel_fifo(int channel);

        void play_start();
        void play_stop();
        void play_continue();
};