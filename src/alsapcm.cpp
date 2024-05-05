#include "common.hpp"
#include "alsa/asoundlib.h"
#include "alsapcm.hpp"

static void* run_pcm(void* context) {
    AlsaPcm& self = *(AlsaPcm*)context;
    while (!self.stop) {
        sleep(1);
    }
    return 0;
}

snd_pcm_t* AlsaPcm::open_pcm_out(const std::string& pcm_out_name) {
    snd_pcm_t* result;
    int err = snd_pcm_open(&result, pcm_out_name.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
    if (err) {
        SPDLOG_ERROR("snd_pcm_open failed = {}", snd_strerror(err));
        return nullptr;
    }
    SPDLOG_INFO("ALSA pcm out open. [{}]", pcm_out_name);
    return result;
}

AlsaPcm::AlsaPcm():
    pcm_out(open_pcm_out(PCM_OUT_NAME)),
    stop(false),
    pcm_thread(create_rt_thread(90, run_pcm, this)) {    
}

AlsaPcm::~AlsaPcm() {    
    stop = true;
    void* status;
    pthread_join(pcm_thread, &status);
    if (pcm_out) {
        snd_pcm_drain(pcm_out);
        snd_pcm_close(pcm_out);
    }
}