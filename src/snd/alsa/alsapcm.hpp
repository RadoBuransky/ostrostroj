#pragma once
#include "farbot/fifo.hpp"

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
        std::chrono::milliseconds period_time;
        snd_pcm_uframes_t period_size;
        uint periods;
        std::atomic_bool stop;
        std::function<bool(PcmEvent&, snd_pcm_state_t, bool)> pcm_event_callback;
        std::function<void(PcmFrame_s24_3le&)> pcm_callback;
        pthread_t pcm_thread;
        snd_pcm_sframes_t current_delay;
        friend void* run_pcm(void* context);
        void run();
        void process_events(bool wait_for_event);
        bool wait_until_avail(bool& wait_for_event);
        void write(snd_pcm_uframes_t size);
        void write_to_mmap(PcmFrame_s24_3le* buffer, snd_pcm_uframes_t frames_to_write);
        void alsa_snd_pcm_mmap_begin(const snd_pcm_channel_area_t **areas, snd_pcm_uframes_t *offset, snd_pcm_uframes_t *frames);
        void alsa_snd_pcm_mmap_commit(snd_pcm_uframes_t offset, snd_pcm_uframes_t frames);
        snd_pcm_state_t alsa_snd_pcm_state();
        void alsa_snd_pcm_start();
        void alsa_snd_pcm_pause();
        void alsa_snd_pcm_resume();
        void alsa_snd_pcm_drop();
        int set_hwparams(snd_pcm_t* handle, snd_pcm_hw_params_t* params, snd_pcm_uframes_t sample_rate);
        int set_swparams(snd_pcm_t* handle, snd_pcm_sw_params_t* swparams);
        void wait_for_device(const std::string& pcm_out_name);
        snd_pcm_t* open_pcm_out(const std::string& pcm_out_name, snd_pcm_uframes_t sample_rate);
    public:
        AlsaPcm(snd_pcm_uframes_t sample_rate);
        virtual ~AlsaPcm();
        void start(std::function<bool(PcmEvent&, snd_pcm_state_t, bool)> _pcm_event_callback, std::function<void(PcmFrame_s24_3le&)> _pcm_callback);
        void shutdown();
        int get_channels() const;
        std::chrono::milliseconds get_period_time();
        snd_pcm_uframes_t get_period_size();
        uint get_periods();
};