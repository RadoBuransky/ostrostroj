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
    inline void silence() {
        b0 = 0;
        b1 = 0;
        b2 = 0;
    }
};

struct PcmFrame_s24_3le {
    std::array<PcmSample_s24_3le,PCM_OUT_CHANNELS> channels;
};

enum PcmEvent {
    ALSA_PCM_START = 0,
    ALSA_PCM_PAUSE,
    ALSA_PCM_RESUME,
    ALSA_PCM_PROGRAM_CHANGE
};

class AlsaPcm {
    private:
        snd_pcm_t* pcm_out;
        snd_pcm_uframes_t buffer_size;
        snd_pcm_uframes_t period_size;
        std::atomic_bool stop;
        std::function<bool(PcmEvent&, bool)> pcm_event_callback;
        std::function<void(PcmFrame_s24_3le&)> pcm_callback;
        pthread_t pcm_thread;
        snd_pcm_sframes_t current_delay;
        friend void* run_pcm(void* context);
        void process_events();
        snd_pcm_state_t alsa_snd_pcm_state();
        void alsa_snd_pcm_start();
        void alsa_snd_pcm_pause();
        void alsa_snd_pcm_resume();
        void alsa_snd_pcm_drop();
        int set_hwparams(snd_pcm_t* handle, snd_pcm_hw_params_t* params);
        int set_swparams(snd_pcm_t* handle, snd_pcm_sw_params_t* swparams);
        snd_pcm_t* open_pcm_out(const std::string& pcm_out_name);
    public:
        AlsaPcm();
        virtual ~AlsaPcm();
        void start(std::function<bool(PcmEvent&, bool)> _pcm_event_callback, std::function<void(PcmFrame_s24_3le&)> _pcm_callback);
        snd_pcm_uframes_t get_sample_rate() const;
        int get_channels() const;
        std::chrono::milliseconds get_period_time();
        snd_pcm_uframes_t get_period_size();
};