#include <alsa/pcm.h>

class AlsaPcm {
    private:
        static constexpr std::string PCM_OUT_NAME = "hw:UMC1820";
        static constexpr snd_pcm_access_t PCM_OUT_ACCESS = SND_PCM_ACCESS_RW_INTERLEAVED;
        static constexpr unsigned int PCM_OUT_RATE = 96000;
        static constexpr snd_pcm_format_t PCM_OUT_FORMAT = SND_PCM_FORMAT_S24_3LE;
        static constexpr unsigned int PCM_OUT_CHANNELS = 12;
        static constexpr unsigned int PCM_OUT_BUFFER_TIME_US = 500000;
        static constexpr unsigned int PCM_OUT_PERIOD_TIME_US = 100000;
        static constexpr int THREAD_PRIORITY = 80;
        snd_pcm_t* pcm_out;
        unsigned int buffer_size;
        unsigned int period_size;
        std::atomic_bool stop;
        pthread_t pcm_thread;
        int set_hwparams(snd_pcm_t* handle, snd_pcm_hw_params_t* params);
        int set_swparams(snd_pcm_t* handle, snd_pcm_sw_params_t* swparams);
        snd_pcm_t* open_pcm_out(const std::string& pcm_out_name);
        friend void* run_pcm(void* context);
    public:
        AlsaPcm();
        virtual ~AlsaPcm();
};