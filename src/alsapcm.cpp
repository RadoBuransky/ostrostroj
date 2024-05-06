#include "common.hpp"
#include "alsa/asoundlib.h"
#include "alsapcm.hpp"

// #define TEST_PARAMS

void* run_pcm(void* context) {
    AlsaPcm& self = *(AlsaPcm*)context;
    const snd_pcm_channel_area_t* areas;
    snd_pcm_state_t state;
    snd_pcm_sframes_t avail, commitres;
    bool first = true;
    int err;
    snd_pcm_uframes_t offset, frames, size;
    float sample;
    unsigned char* buffer;
    int step;

    while (!self.stop) {
        state = snd_pcm_state(self.pcm_out);
        if (state == SND_PCM_STATE_XRUN || state == SND_PCM_STATE_SUSPENDED) {            
            // TODO: Handle xrun
            SPDLOG_ERROR("Invalid state! [{}]", (int)state);
            return 0;
        }
        avail = snd_pcm_avail_update(self.pcm_out);
        if (avail < 0) {
            // TODO: Handle xrun
            first = true;
            continue;
        }
        if (avail < (snd_pcm_sframes_t)self.period_size) {
            if (first) {
                first = false;
                err = snd_pcm_start(self.pcm_out);
                if (err < 0) {
                    SPDLOG_ERROR("snd_pcm_start failed = {}", snd_strerror(err));
                    return 0;
                }
                SPDLOG_INFO("PCM started.");
            } else {
                err = snd_pcm_wait(self.pcm_out, -1);
                if (err < 0) {
                    SPDLOG_ERROR("snd_pcm_wait failed = {}", snd_strerror(err));
                    // TODO: Handle xrun
                    first = true;
                    return 0;
                }
            }
            continue;
        }
        size = self.period_size;
        while (size > 0) {
            frames = size;
            err = snd_pcm_mmap_begin(self.pcm_out, &areas, &offset, &frames);
            if (err < 0) {
                SPDLOG_ERROR("snd_pcm_mmap_begin failed = {}", snd_strerror(err));
                // TODO: Handle xrun
                first = true;
            }
            
            for (int channel = 0; channel < self.PCM_OUT_CHANNELS; channel++) {
                PcmFifo& pcm_fifo = *self.channel_fifos.at(channel);
                step = areas[channel].step / 8;
                buffer = ((unsigned char*)areas[channel].addr) + (areas[channel].first / 8) + (offset * step);                
                while (frames-- > 0) {
                    if (!pcm_fifo.pop(sample)) {
                        SPDLOG_ERROR("Channel {} xrun!", channel);
                        sample = 0.0;
                    }
                    self.float_to_s24_3le(sample, buffer);
                    buffer += step;
                }
            }

            commitres = snd_pcm_mmap_commit(self.pcm_out, offset, frames);            
            if (commitres < 0 || (snd_pcm_uframes_t)commitres != frames) {
                // TODO: Handle xrun
                SPDLOG_ERROR("commit {} xrun!", commitres);
                first = true;
            }
            size -= frames;
        }
        self.next_period_flag.clear();
        self.next_period_flag.notify_all();
    }

    return 0;
}

// https://github.com/naudio/NAudio/blob/a106da4eed61774e9bd3eda1fa7922581aee04e1/NAudio.Asio/ASIOSampleConvertor.cs#L415
void AlsaPcm::float_to_s24_3le(float sample, unsigned char* buffer) {
    signed int sample24 = (signed int)((double)sample * (double)8388607.0);
    buffer[0] = (unsigned char)(sample24);
    buffer[1] = (unsigned char)(sample24 >> 8);
    buffer[2] = (unsigned char)(sample24 >> 16);
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
    unsigned int rbuffer_time = std::chrono::duration<long, std::micro>(PCM_OUT_BUFFER_TIME).count();
    err = snd_pcm_hw_params_set_buffer_time_near(handle, params, &rbuffer_time, &dir);
    if (err < 0) {
        SPDLOG_ERROR("Unable to set buffer time {} for playback: {}", std::to_string(PCM_OUT_BUFFER_TIME.count()), snd_strerror(err));
        return err;
    }
    err = snd_pcm_hw_params_get_buffer_size(params, &size);
    if (err < 0) {
        SPDLOG_ERROR("Unable to get buffer size for playback: {}", snd_strerror(err));
        return err;
    }
    buffer_size = size;
    unsigned int rperiod_time =  std::chrono::duration<long, std::micro>(PCM_OUT_PERIOD_TIME).count();
    err = snd_pcm_hw_params_set_period_time_near(handle, params, &rperiod_time, &dir);
    if (err < 0) {
        SPDLOG_ERROR("Unable to set period time {} for playback: {}", std::to_string(PCM_OUT_PERIOD_TIME.count()), snd_strerror(err));
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
    SPDLOG_INFO("ALSA pcm out hwparams set. [buffer_size={} frames,period_size={} frames]", buffer_size, period_size);
    snd_pcm_sw_params_t *swparams;
    snd_pcm_sw_params_alloca(&swparams);
    err = set_swparams(result, swparams);
    if (err < 0) {
        SPDLOG_ERROR("Setting of swparams failed: {}", snd_strerror(err));
    }
    SPDLOG_INFO("ALSA pcm out swparams set.");
    return result;
}

std::vector<std::unique_ptr<PcmFifo>> AlsaPcm::create_channel_fifos() {
    std::vector<std::unique_ptr<PcmFifo>> result;
    result.reserve(PCM_OUT_CHANNELS);
    for (unsigned int i = 0; i < PCM_OUT_CHANNELS; i++) {
        result.emplace_back(std::make_unique<PcmFifo>(period_size));
    }
    return result;
}

AlsaPcm::AlsaPcm():
    pcm_out(open_pcm_out(PCM_OUT_NAME)),
    stop(false),
    channel_fifos(create_channel_fifos()),
    next_period_flag(ATOMIC_FLAG_INIT),
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

snd_pcm_uframes_t AlsaPcm::get_sample_rate() const {
    return PCM_OUT_RATE;
}

int AlsaPcm::get_channels() const {
    return PCM_OUT_CHANNELS;    
}

PcmFifo& AlsaPcm::get_channel_fifo(int channel) {
    return *channel_fifos.at(channel);
}

std::atomic_flag& AlsaPcm::get_next_period_flag() {
    return next_period_flag;
}