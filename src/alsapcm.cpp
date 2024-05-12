#define SPDLOG_ACTIVE_LEVEL 1

#include "common.hpp"
#include "alsa/asoundlib.h"
#include "alsapcm.hpp"

// #define TEST_PARAMS

static constexpr std::string PCM_OUT_NAME = "hw:UMC1820";
static constexpr snd_pcm_access_t PCM_OUT_ACCESS = SND_PCM_ACCESS_MMAP_INTERLEAVED;
static constexpr snd_pcm_uframes_t PCM_OUT_RATE = 96000;
static constexpr snd_pcm_format_t PCM_OUT_FORMAT = SND_PCM_FORMAT_S24_3LE;
static constexpr std::chrono::duration<long, std::milli> PCM_OUT_PERIOD_TIME = std::chrono::milliseconds(10);
static constexpr int PCM_OUT_BUFFER_PERIODS = 100;
static constexpr int THREAD_PRIORITY = 80;

PcmSample_s24_3le::PcmSample_s24_3le(float sample) {
    *this = sample;
};

PcmSample_s24_3le& PcmSample_s24_3le::operator=(float sample) {
    // https://github.com/naudio/NAudio/blob/a106da4eed61774e9bd3eda1fa7922581aee04e1/NAudio.Asio/ASIOSampleConvertor.cs#L415
    signed int sample24 = (signed int)((double)sample * (double)8388607.0);
    b0 = (unsigned char)(sample24);
    b1 = (unsigned char)(sample24 >> 8);
    b2 = (unsigned char)(sample24 >> 16);
    return *this;
}

void* run_pcm(void* context) {
    ((AlsaPcm*)context)->run();
    return 0;
}

void AlsaPcm::run() {    
    SPDLOG_INFO("ALSA PCM started.");
    try {
        snd_pcm_uframes_t total_frames_written = 0;
        bool wait_for_event = false;
        while (!stop) {
            process_events(wait_for_event);
            if (wait_until_avail(wait_for_event)) {
                write(period_size);
                total_frames_written += period_size;
            }
        }
        SPDLOG_INFO("ALSA PCM stopped.");
    } catch(std::exception const& e) {
        SPDLOG_ERROR("ALSA PCM failed {}", e.what());
    }
}

void AlsaPcm::process_events(bool wait_for_event) {
    PcmEvent event;
    snd_pcm_state_t state = alsa_snd_pcm_state();
    while (pcm_event_callback(event, state == SND_PCM_STATE_RUNNING, wait_for_event)) {
        SPDLOG_DEBUG("ALSA PCM event = {}", (int)event);
        switch(event) {
            case ALSA_PCM_START:
                if (state == SND_PCM_STATE_PREPARED) {
                    alsa_snd_pcm_start();
                } else {
                    SPDLOG_WARN("Invalid ALSA_PCM_START. [state={}]", (int)state);
                }
                break;
            case ALSA_PCM_PAUSE:
                if (state == SND_PCM_STATE_RUNNING) {
                    alsa_snd_pcm_pause();
                } else {
                    SPDLOG_WARN("Invalid ALSA_PCM_PAUSE. [state={}]", (int)state);
                }
                break;
            case ALSA_PCM_RESUME:
                if (state == SND_PCM_STATE_PAUSED) {
                    alsa_snd_pcm_resume();
                } else {
                    SPDLOG_WARN("Invalid ALSA_PCM_RESUME. [state={}]", (int)state);
                }
                break;
            case ALSA_PCM_PROGRAM_CHANGE:
                if (state == SND_PCM_STATE_RUNNING || state == SND_PCM_STATE_PAUSED) {
                    SPDLOG_DEBUG("ALSA_PCM_PROGRAM_CHANGE while running.");
                    break;
                }
                alsa_snd_pcm_drop();
                break;
            default:
                SPDLOG_ERROR("Unknown event! [{}]", (int)event);
                break;
        }
    }
}

bool AlsaPcm::wait_until_avail(bool& wait_for_event) {
    snd_pcm_sframes_t avail;
    wait_for_event = false;
    int err = snd_pcm_avail_delay(pcm_out, &avail, &current_delay);
    if (err < 0 || avail < 0) {
        throw OstrostrojException(fmt::format("snd_pcm_avail_delay failed = [err={},avail={}]", snd_strerror(err), avail));
    }
    snd_pcm_state_t state = alsa_snd_pcm_state();
    if (state == SND_PCM_STATE_XRUN || state == SND_PCM_STATE_SUSPENDED) {
        throw OstrostrojException(fmt::format("Invalid state={}", (int)state));
    }
    if (avail < (snd_pcm_sframes_t)period_size) {
        if (state != SND_PCM_STATE_RUNNING) {
            wait_for_event = true;
        } else {
            SPDLOG_TRACE("ALSA PCM busy loop. [state={}]", (int)state);
            usleep(std::chrono::microseconds(PCM_OUT_PERIOD_TIME).count());
        }
        return false;
    }
    return true;
}

void AlsaPcm::write(snd_pcm_uframes_t size) {
    const snd_pcm_channel_area_t* areas;
    snd_pcm_uframes_t offset, frames;
    PcmFrame_s24_3le* buffer;
    SPDLOG_TRACE("ALSA PCM writing {} frames...", period_size);
    while (size > 0) {
        frames = size;
        alsa_snd_pcm_mmap_begin(&areas, &offset, &frames);
        buffer = (PcmFrame_s24_3le*)(((char*)areas[0].addr) + (areas[0].first / 8)) + offset;
        write_to_mmap(buffer, frames);
        alsa_snd_pcm_mmap_commit(offset, frames);
        size -= frames;
    }
    SPDLOG_TRACE("ALSA PCM frames commited");
}

void AlsaPcm::write_to_mmap(PcmFrame_s24_3le* buffer, snd_pcm_uframes_t frames_to_write) {
    while (frames_to_write-- > 0) {
        pcm_callback(*buffer);
        buffer++;
    }
}

void AlsaPcm::alsa_snd_pcm_mmap_begin(const snd_pcm_channel_area_t **areas, snd_pcm_uframes_t *offset, snd_pcm_uframes_t *frames) {
    int err = snd_pcm_mmap_begin(pcm_out, areas, offset, frames);
    SPDLOG_TRACE("snd_pcm_mmap_begin = {}, {}, {}", err, offset, frames);
    if (err < 0) {
        throw OstrostrojException(fmt::format("snd_pcm_mmap_begin failed={}", snd_strerror(err)));
    }
}

void AlsaPcm::alsa_snd_pcm_mmap_commit(snd_pcm_uframes_t offset, snd_pcm_uframes_t frames) {
    snd_pcm_sframes_t commitres = snd_pcm_mmap_commit(pcm_out, offset, frames);
    SPDLOG_TRACE("snd_pcm_mmap_commit = {}, {}, {}", commitres, offset, frames);       
    if (commitres < 0 || (snd_pcm_uframes_t)commitres != frames) {
        throw OstrostrojException(fmt::format("snd_pcm_mmap_commit failed={}", commitres));
    }
}

snd_pcm_state_t AlsaPcm::alsa_snd_pcm_state() {
    snd_pcm_state_t result = snd_pcm_state(pcm_out);
    SPDLOG_TRACE("snd_pcm_state = {}", (long)result);
    if (result < 0) {
        throw OstrostrojException(fmt::format("snd_pcm_state failed={}", snd_strerror(result)));
    }
    return result;
}

void AlsaPcm::alsa_snd_pcm_start() {
    int err = snd_pcm_start(pcm_out);
    if (err < 0) {
        throw OstrostrojException(fmt::format("snd_pcm_start failed={}", snd_strerror(err)));
    }
}

void AlsaPcm::alsa_snd_pcm_pause() {    
    int err = snd_pcm_pause(pcm_out, true);
    if (err < 0) {
        throw OstrostrojException(fmt::format("snd_pcm_pause (pause) failed={}", snd_strerror(err)));
    }
}

void AlsaPcm::alsa_snd_pcm_resume() {  
    int err = snd_pcm_pause(pcm_out, false);
    if (err < 0) {
        throw OstrostrojException(fmt::format("snd_pcm_pause (resume) failed={}", snd_strerror(err)));
    }  
}

void AlsaPcm::alsa_snd_pcm_drop() {
    int err = snd_pcm_drop(pcm_out);
    if (err < 0) {
        throw OstrostrojException(fmt::format("snd_pcm_drop failed={}", snd_strerror(err)));
    }
    err = snd_pcm_prepare(pcm_out);
    if (err < 0) {
        throw OstrostrojException(fmt::format("snd_pcm_prepare failed={}", snd_strerror(err)));
    }
    snd_pcm_state_t state = alsa_snd_pcm_state();
    if (state != SND_PCM_STATE_PREPARED) {
        throw OstrostrojException(fmt::format("Not in SND_PCM_STATE_PREPARED state. [{}]", (int)state));        
    }
}
 
int AlsaPcm::set_hwparams(snd_pcm_t* handle, snd_pcm_hw_params_t* params) {
    unsigned int rrate;
    snd_pcm_uframes_t size;
    int err, dir;
    err = snd_pcm_hw_params_any(handle, params);
    if (err < 0) {
        throw OstrostrojException(fmt::format("Broken configuration for playback: no configurations available: {}", snd_strerror(err)));
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
        throw OstrostrojException(fmt::format("Access type not available for playback: {}", snd_strerror(err)));
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
        throw OstrostrojException(fmt::format("Sample format not available for playback: {}", snd_strerror(err)));
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
        throw OstrostrojException(fmt::format("Channels count ({}) not available for playbacks: {}", PCM_OUT_CHANNELS, snd_strerror(err)));
    }
    rrate = PCM_OUT_RATE;
    err = snd_pcm_hw_params_set_rate_near(handle, params, &rrate, 0);
    if (err < 0) {
        throw OstrostrojException(fmt::format("Rate {}Hz not available for playback: {}", PCM_OUT_RATE, snd_strerror(err)));
    }
    if (rrate != PCM_OUT_RATE) {
        throw OstrostrojException(fmt::format("Rate doesn't match (requested {}Hz, get {}Hz)", PCM_OUT_RATE, err));
    }
    size = PCM_OUT_PERIOD_TIME.count() * PCM_OUT_RATE / 1000;
    period_size = 1;
    while (period_size < size) {
        period_size *= 2;
    }
    err = snd_pcm_hw_params_set_period_size(handle, params, period_size, 0);
    if (err < 0) {
        snd_pcm_uframes_t min, max;
        snd_pcm_hw_params_get_period_size_min(params, &min, &dir);
        snd_pcm_hw_params_get_period_size_max(params, &max, &dir);

        throw OstrostrojException(fmt::format("snd_pcm_hw_params_set_period_size failed = {} [period_size={} <{}:{}>]", snd_strerror(err), period_size, min, max));
    }
    err = snd_pcm_hw_params_get_period_size(params, &period_size, &dir);
    if (err < 0) {
        throw OstrostrojException(fmt::format("snd_pcm_hw_params_get_period_size failed = {}", snd_strerror(err)));
    }

    err = snd_pcm_hw_params_set_buffer_size(handle, params, period_size * PCM_OUT_BUFFER_PERIODS);
    if (err < 0) {
        throw OstrostrojException(fmt::format("snd_pcm_hw_params_set_buffer_size failed = {}", snd_strerror(err)));
    }
    err = snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
    if (err < 0) {
        throw OstrostrojException(fmt::format("snd_pcm_hw_params_get_buffer_size failed = {}", snd_strerror(err)));
    }
    err = snd_pcm_hw_params_can_pause(params);
    if (err < 0) {
        throw OstrostrojException(fmt::format("snd_pcm_hw_params_can_pause failed = {}", snd_strerror(err)));
    }
    err = snd_pcm_hw_params(handle, params);
    if (err < 0) {
        throw OstrostrojException(fmt::format("Unable to set hw params for playback: {}", snd_strerror(err)));
    }
    return 0;
}
 
int AlsaPcm::set_swparams(snd_pcm_t* handle, snd_pcm_sw_params_t* swparams) {
    int err; 
    // get the current swparams
    err = snd_pcm_sw_params_current(handle, swparams);
    if (err < 0) {
        throw OstrostrojException(fmt::format("Unable to determine current swparams for playback: {}", snd_strerror(err)));
    }
    // start the transfer when the buffer is almost full:
    // (buffer_size / avail_min) * avail_min
    // err = snd_pcm_sw_params_set_start_threshold(handle, swparams, (buffer_size / period_size) * period_size);
    // if (err < 0) {
    //     SPDLOG_ERROR("Unable to set start threshold mode for playback: {}", snd_strerror(err));
    //     return err;
    // }
    // allow the transfer when at least period_size samples can be processed
    // or disable this mechanism when period event is enabled (aka interrupt like style processing)
    // int period_event = 0;
    err = snd_pcm_sw_params_set_avail_min(handle, swparams, 1);
    if (err < 0) {
        throw OstrostrojException(fmt::format("snd_pcm_sw_params_set_avail_min failed = {}", snd_strerror(err)));
    }
    // enable period events when requested
    // if (period_event) {
    //     err = snd_pcm_sw_params_set_period_event(handle, swparams, 1);
    //     if (err < 0) {
    //         SPDLOG_ERROR("Unable to set period event: {}", snd_strerror(err));
    //         return err;
    //     }
    // }
    // write the parameters to the playback device
    err = snd_pcm_sw_params(handle, swparams);
    if (err < 0) {
        throw OstrostrojException(fmt::format("Unable to set sw params for playback: {}", snd_strerror(err)));
    }
    return 0;
}

snd_pcm_t* AlsaPcm::open_pcm_out(const std::string& pcm_out_name) {
    snd_pcm_t* result;
    int err = snd_pcm_open(&result, pcm_out_name.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
    if (err) {
        throw OstrostrojException(fmt::format("snd_pcm_open failed = {}", snd_strerror(err)));
    }
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_hw_params_alloca(&hwparams);
    err = set_hwparams(result, hwparams);
    if (err < 0) {
        throw OstrostrojException(fmt::format("Setting of hwparams failed: {}", snd_strerror(err)));
    }
    snd_pcm_sw_params_t *swparams;
    snd_pcm_sw_params_alloca(&swparams);
    err = set_swparams(result, swparams);
    if (err < 0) {
        throw OstrostrojException(fmt::format("Setting of swparams failed: {}", snd_strerror(err)));
    }
    const char* pcm_name = snd_pcm_name(result);
    snd_pcm_type_t pcm_type = snd_pcm_type(result);
    snd_pcm_sframes_t delay;
    snd_pcm_delay(result, &delay);
    snd_pcm_uframes_t start_threshold;
    snd_pcm_sw_params_get_start_threshold(swparams, &start_threshold);
    snd_pcm_uframes_t avail_min;
    snd_pcm_sw_params_get_avail_min(swparams, &avail_min);
    SPDLOG_INFO("ALSA pcm out open. [name={},buffer_size={},period_size={},type={},delay={},start_threshold={},avail_min={},sample size={}]",
        pcm_name, buffer_size, period_size, (int)pcm_type, delay, start_threshold, avail_min, sizeof(PcmSample_s24_3le));
    return result;
}

AlsaPcm::AlsaPcm():
    pcm_out(open_pcm_out(PCM_OUT_NAME)),
    stop(false),
    pcm_event_callback(0),
    pcm_callback(0),
    pcm_thread(0){    
}

AlsaPcm::~AlsaPcm() {
    if (pcm_thread) {
        stop = true;
        void* status;
        pthread_join(pcm_thread, &status);
    }
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

std::chrono::milliseconds AlsaPcm::get_period_time() {
    return PCM_OUT_PERIOD_TIME;
}

snd_pcm_uframes_t AlsaPcm::get_period_size() {
    return period_size;
}

void AlsaPcm::start(std::function<bool(PcmEvent&, bool, bool)> _pcm_event_callback, std::function<void(PcmFrame_s24_3le&)> _pcm_callback) {
    if (pcm_thread || pcm_callback) {
        SPDLOG_ERROR("Thread already started! [{}]", pcm_thread);
        return;
    }
    pcm_event_callback = _pcm_event_callback;
    pcm_callback = _pcm_callback;
    pcm_thread = create_rt_thread("alsa_pcm", THREAD_PRIORITY, run_pcm, this);
}