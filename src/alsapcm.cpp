#include "common.hpp"
#include "alsa/asoundlib.h"
#include "alsapcm.hpp"

// #define TEST_PARAMS

void* run_pcm(void* context) {
    AlsaPcm& self = *(AlsaPcm*)context;
    while (!self.stop) {
        sleep(1);
    }
    return 0;
}
 
int AlsaPcm::set_hwparams(snd_pcm_t* handle, snd_pcm_hw_params_t* params) {
    unsigned int rrate;
    snd_pcm_uframes_t size;
    int err, dir;
    err = snd_pcm_hw_params_any(handle, params);
    if (err < 0) {
        SPDLOG_ERROR("Broken configuration for playback: no configurations available: {}", snd_strerror(err));
        return err;
    }
#ifdef TEST_PARAMS
    for (int f = SND_PCM_ACCESS_MMAP_INTERLEAVED; f < SND_PCM_ACCESS_LAST; f++) {
        err = snd_pcm_hw_params_test_access(handle, params, static_cast<snd_pcm_access_t>(f));
        if (err == 0) {
            SPDLOG_INFO("Available access = {}", f);
        }
    }
#endif
    err = snd_pcm_hw_params_set_access(handle, params, PCM_OUT_ACCESS);
    if (err < 0) {
        SPDLOG_ERROR("Access type not available for playback: {}", snd_strerror(err));
        return err;
    }
#ifdef TEST_PARAMS
    for (int f = SND_PCM_FORMAT_S8; f < SND_PCM_FORMAT_LAST; f++) {
        err = snd_pcm_hw_params_test_format(handle, params, static_cast<snd_pcm_format_t>(f));
        if (err == 0) {
            SPDLOG_INFO("Available format = {}", f);
        }
    }
#endif
    err = snd_pcm_hw_params_set_format(handle, params, PCM_OUT_FORMAT);
    if (err < 0) {
        SPDLOG_ERROR("Sample format not available for playback: {}", snd_strerror(err));
        return err;
    }
#ifdef TEST_PARAMS
    for (int ch = 1; ch < 32; ch++ ) {
        err = snd_pcm_hw_params_test_channels(handle, params, ch);
        if (err == 0) {
            SPDLOG_INFO("Available channels = {}", ch);
        }
    }
#endif
    err = snd_pcm_hw_params_set_channels(handle, params, PCM_OUT_CHANNELS);
    if (err < 0) {
        SPDLOG_ERROR("Channels count ({}) not available for playbacks: {}", PCM_OUT_CHANNELS, snd_strerror(err));
        return err;
    }
    rrate = PCM_OUT_RATE;
    err = snd_pcm_hw_params_set_rate_near(handle, params, &rrate, 0);
    if (err < 0) {
        SPDLOG_ERROR("Rate {}Hz not available for playback: {}", PCM_OUT_RATE, snd_strerror(err));
        return err;
    }
    if (rrate != PCM_OUT_RATE) {
        SPDLOG_ERROR("Rate doesn't match (requested {}Hz, get {}Hz)", PCM_OUT_RATE, err);
        return -EINVAL;
    }
    unsigned int rbuffer_time = PCM_OUT_BUFFER_TIME_US;
    err = snd_pcm_hw_params_set_buffer_time_near(handle, params, &rbuffer_time, &dir);
    if (err < 0) {
        SPDLOG_ERROR("Unable to set buffer time {} for playback: {}", PCM_OUT_BUFFER_TIME_US, snd_strerror(err));
        return err;
    }
    err = snd_pcm_hw_params_get_buffer_size(params, &size);
    if (err < 0) {
        SPDLOG_ERROR("Unable to get buffer size for playback: {}", snd_strerror(err));
        return err;
    }
    buffer_size = size;
    unsigned int rperiod_time = PCM_OUT_PERIOD_TIME_US;
    err = snd_pcm_hw_params_set_period_time_near(handle, params, &rperiod_time, &dir);
    if (err < 0) {
        SPDLOG_ERROR("Unable to set period time {} for playback: {}", PCM_OUT_PERIOD_TIME_US, snd_strerror(err));
        return err;
    }
    err = snd_pcm_hw_params_get_period_size(params, &size, &dir);
    if (err < 0) {
        SPDLOG_ERROR("Unable to get period size for playback: {}", snd_strerror(err));
        return err;
    }
    period_size = size;
    err = snd_pcm_hw_params(handle, params);
    if (err < 0) {
        SPDLOG_ERROR("Unable to set hw params for playback: {}", snd_strerror(err));
        return err;
    }
    return 0;
}
 
int AlsaPcm::set_swparams(snd_pcm_t* handle, snd_pcm_sw_params_t* swparams) {
    int err; 
    // get the current swparams
    err = snd_pcm_sw_params_current(handle, swparams);
    if (err < 0) {
        SPDLOG_ERROR("Unable to determine current swparams for playback: {}", snd_strerror(err));
        return err;
    }
    // start the transfer when the buffer is almost full:
    // (buffer_size / avail_min) * avail_min
    err = snd_pcm_sw_params_set_start_threshold(handle, swparams, (buffer_size / period_size) * period_size);
    if (err < 0) {
        SPDLOG_ERROR("Unable to set start threshold mode for playback: {}", snd_strerror(err));
        return err;
    }
    // allow the transfer when at least period_size samples can be processed
    // or disable this mechanism when period event is enabled (aka interrupt like style processing)
    int period_event = 0;
    err = snd_pcm_sw_params_set_avail_min(handle, swparams, period_event ? buffer_size : period_size);
    if (err < 0) {
        SPDLOG_ERROR("Unable to set avail min for playback: {}", snd_strerror(err));
        return err;
    }
    // enable period events when requested
    if (period_event) {
        err = snd_pcm_sw_params_set_period_event(handle, swparams, 1);
        if (err < 0) {
            SPDLOG_ERROR("Unable to set period event: {}", snd_strerror(err));
            return err;
        }
    }
    // write the parameters to the playback device
    err = snd_pcm_sw_params(handle, swparams);
    if (err < 0) {
        SPDLOG_ERROR("Unable to set sw params for playback: {}", snd_strerror(err));
        return err;
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
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_hw_params_alloca(&hwparams);
    err = set_hwparams(result, hwparams);
    if (err < 0) {
        SPDLOG_ERROR("Setting of hwparams failed: {}", snd_strerror(err));
    }
    SPDLOG_INFO("ALSA pcm out hwparams set.");
    snd_pcm_sw_params_t *swparams;
    snd_pcm_sw_params_alloca(&swparams);
    err = set_swparams(result, swparams);
    if (err < 0) {
        SPDLOG_ERROR("Setting of swparams failed: {}", snd_strerror(err));
    }
    SPDLOG_INFO("ALSA pcm out swparams set.");
    return result;
}

AlsaPcm::AlsaPcm():
    pcm_out(open_pcm_out(PCM_OUT_NAME)),
    stop(false),
    pcm_thread(create_rt_thread(THREAD_PRIORITY, run_pcm, this)) {    
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