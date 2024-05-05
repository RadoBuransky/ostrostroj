#include <alsa/pcm.h>

static void* run_pcm(void* context);

class AlsaPcm {
    private:
        static constexpr std::string PCM_OUT_NAME = "hw:UMC1820";
        snd_pcm_t* pcm_out;
        std::atomic_bool stop;
        pthread_t pcm_thread;
        snd_pcm_t* open_pcm_out(const std::string& pcm_out_name);
        friend void* run_pcm(void* context);
    public:
        AlsaPcm();
        virtual ~AlsaPcm();
};