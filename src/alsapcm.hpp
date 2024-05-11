#pragma once
#include <alsa/pcm.h>
#include "farbot/fifo.hpp"

static constexpr int PCM_OUT_CHANNELS = 12;

struct PcmSample_s24_3le {
    unsigned char b0;
    unsigned char b1;
    unsigned char b2;
    PcmSample_s24_3le() = default;
    PcmSample_s24_3le(float sample);
    PcmSample_s24_3le& operator=(float sample);
    void silence();
};

struct PcmFrame_s24_3le {
    std::array<PcmSample_s24_3le,PCM_OUT_CHANNELS> channels;
    void silence();
};

typedef farbot::fifo<PcmFrame_s24_3le,
            farbot::fifo_options::concurrency::single,
            farbot::fifo_options::concurrency::single,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty> PcmFifo;

enum PcmEvent {
    ALSA_PCM_START = 0,
    ALSA_PCM_STOP,
    ALSA_PCM_CONTINUE
};

typedef farbot::fifo<PcmEvent,
            farbot::fifo_options::concurrency::single,
            farbot::fifo_options::concurrency::single,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty> PcmEventFifo;

class AlsaPcm {
    private:
        snd_pcm_t* pcm_out;
        snd_pcm_uframes_t buffer_size;
        snd_pcm_uframes_t period_size;
        std::atomic_bool stop;
        std::unique_ptr<PcmFifo> pcm_fifo;
        std::function<void(void)> callback;
        std::unique_ptr<PcmEventFifo> pcm_event_fifo;
        std::atomic_flag pcm_event_pushed_flag;
        pthread_t pcm_thread;
        friend void* run_pcm(void* context);
        void process_events();
        void push_pcm_event(PcmEvent&& pcm_event);
        int set_hwparams(snd_pcm_t* handle, snd_pcm_hw_params_t* params);
        int set_swparams(snd_pcm_t* handle, snd_pcm_sw_params_t* swparams);
        snd_pcm_t* open_pcm_out(const std::string& pcm_out_name);
        std::unique_ptr<PcmFifo> create_pcm_fifo();
    public:
        AlsaPcm();
        virtual ~AlsaPcm();
        void start(std::function<void(void)> _callback);
        snd_pcm_uframes_t get_sample_rate() const;
        int get_channels() const;
        std::chrono::milliseconds get_period_time();
        snd_pcm_uframes_t get_period_size();
        PcmFifo& get_pcm_fifo();

        void play_start();
        void play_stop();
        void play_continue();
};