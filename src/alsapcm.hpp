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
        static constexpr std::string PCM_OUT_NAME = "hw:UMC1820";
        static constexpr snd_pcm_access_t PCM_OUT_ACCESS = SND_PCM_ACCESS_MMAP_INTERLEAVED;
        static constexpr snd_pcm_uframes_t PCM_OUT_RATE = 96000;
        static constexpr snd_pcm_format_t PCM_OUT_FORMAT = SND_PCM_FORMAT_S24_3LE;
        static constexpr int PCM_OUT_CHANNELS = 12;
        static constexpr std::chrono::duration<long, std::milli> PCM_OUT_BUFFER_TIME = std::chrono::milliseconds(5);
        static constexpr std::chrono::duration<long, std::milli> PCM_OUT_PERIOD_TIME = std::chrono::milliseconds(1);
        static constexpr int THREAD_PRIORITY = 80;
        snd_pcm_t* pcm_out;
        snd_pcm_uframes_t buffer_size;
        snd_pcm_uframes_t period_size;
        std::atomic_bool stop;
        const std::vector<std::unique_ptr<PcmFifo>> channel_fifos;
        std::atomic_flag next_period_flag;
        const pthread_t pcm_thread;
        friend void* run_pcm(void* context);
        void float_to_s24_3le(float sample, unsigned char* buffer);
        int set_hwparams(snd_pcm_t* handle, snd_pcm_hw_params_t* params);
        int set_swparams(snd_pcm_t* handle, snd_pcm_sw_params_t* swparams);
        snd_pcm_t* open_pcm_out(const std::string& pcm_out_name);
        std::vector<std::unique_ptr<PcmFifo>> create_channel_fifos();
    public:
        AlsaPcm();
        virtual ~AlsaPcm();

        snd_pcm_uframes_t get_sample_rate() const;
        int get_channels() const;
        PcmFifo& get_channel_fifo(int channel);
        std::atomic_flag& get_next_period_flag();
};