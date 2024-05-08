#define SPDLOG_ACTIVE_LEVEL 1

#include "common.hpp"
#include "alsa/asoundlib.h"
#include "alsapcm.hpp"

// #define TEST_PARAMS

static constexpr std::string PCM_OUT_NAME = "hw:UMC1820";
static constexpr snd_pcm_access_t PCM_OUT_ACCESS = SND_PCM_ACCESS_MMAP_INTERLEAVED;
static constexpr snd_pcm_uframes_t PCM_OUT_RATE = 96000;
static constexpr snd_pcm_format_t PCM_OUT_FORMAT = SND_PCM_FORMAT_S24_3LE;
static constexpr std::chrono::duration<long, std::milli> PCM_OUT_PERIOD_TIME = std::chrono::milliseconds(25);
static constexpr int PCM_OUT_BUFFER_PERIODS = 4;
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

void PcmSample_s24_3le::silence() {
    b0 = 0;
    b1 = 0;
    b2 = 0;
}

void PcmFrame_s24_3le::silence() {
    for (PcmSample_s24_3le& s: channels) {
        s.silence();
    }
}

void* run_pcm(void* context) {
    AlsaPcm& self = *(AlsaPcm*)context;
    PcmFifo& pcm_fifo = *self.pcm_fifo.get();
    const snd_pcm_channel_area_t* areas;
    snd_pcm_state_t state;
    snd_pcm_sframes_t avail, delay, commitres, size, frames_to_write;
    int err;
    snd_pcm_uframes_t offset, frames;
    bool engine_xrun;
    PcmFrame_s24_3le* buffer;
    SPDLOG_INFO("ALSA pcm started.");

    while (!self.stop) {
        self.process_events();
        state = snd_pcm_state(self.pcm_out);
        SPDLOG_TRACE("state = {}", (long)state);
        if (state == SND_PCM_STATE_XRUN || state == SND_PCM_STATE_SUSPENDED) {            
            // TODO: Handle xrun
            SPDLOG_ERROR("Invalid state! [{}]", (int)state);
            return 0;
        }
        err = snd_pcm_avail_delay(self.pcm_out, &avail, &delay);
        if (err < 0) {
            SPDLOG_ERROR("snd_pcm_avail_delay failed = {}", snd_strerror(err));
            return 0;
        }
        SPDLOG_TRACE("snd_pcm_avail_update = {}", (long)avail);
        if (avail < 0) {
            // TODO: Handle xrun
            continue;
        }
        if (avail < (snd_pcm_sframes_t)self.period_size) {
            state = snd_pcm_state(self.pcm_out);
            SPDLOG_DEBUG("snd_pcm_wait... [state={},avail={},delay={}]", (int)state, avail, delay);
            err = snd_pcm_wait(self.pcm_out, -1);
            SPDLOG_TRACE("snd_pcm_wait = {}", err);
            if (err < 0) {
                SPDLOG_ERROR("snd_pcm_wait failed = {}", snd_strerror(err));
                // TODO: Handle xrun
                return 0;
            }
            continue;
        }
        size = self.period_size;
        engine_xrun = false;
        SPDLOG_DEBUG("ALSA PCM writing {} frames...", self.period_size);
        while (size > 0) {
            frames = size;
            err = snd_pcm_mmap_begin(self.pcm_out, &areas, &offset, &frames);
            SPDLOG_TRACE("snd_pcm_mmap_begin = {}, {}, {}", err, offset, frames);
            if (err < 0) {
                SPDLOG_ERROR("snd_pcm_mmap_begin failed = {}", snd_strerror(err));
                // TODO: Handle xrun
            }
            
            frames_to_write = frames;
            buffer = (PcmFrame_s24_3le*)(((char*)areas[0].addr) + (areas[0].first / 8) + (offset * sizeof(PcmFrame_s24_3le)));
#ifndef NDEBUG
            if (areas[0].step != sizeof(PcmFrame_s24_3le) * 8) {
                SPDLOG_ERROR(fmt::format("Invalid step size! [expected={},actual={}]", sizeof(PcmFrame_s24_3le) * 8, areas[0].step));
                return 0;
            }
            static bool area_debug_logged = false;
            if (!area_debug_logged) {
                area_debug_logged = true;
                for (int channel = 0; channel < PCM_OUT_CHANNELS; channel++) {
                    SPDLOG_TRACE("ch={} area.addr=0x{:x}, area.first={}, area.step={}", channel, (long)areas[channel].addr, areas[channel].first, areas[channel].step);
                }            
            }
#endif            
            while (frames_to_write-- > 0) {
                if (!pcm_fifo.pop(*buffer)) {
                    engine_xrun = true;
                    SPDLOG_WARN("ALSA PCM FIFO xrun...");
                    do {
                        self.callback();
                        usleep(std::chrono::microseconds(PCM_OUT_PERIOD_TIME).count() / 2);
                    } while (!pcm_fifo.pop(*buffer));
                    SPDLOG_WARN("ALSA PCM FIFO recovered.");
                }
                buffer++;
            }
            commitres = snd_pcm_mmap_commit(self.pcm_out, offset, frames);
            SPDLOG_TRACE("snd_pcm_mmap_commit = {}, {}, {}", commitres, offset, frames);       
            if (commitres < 0 || (snd_pcm_uframes_t)commitres != frames) {
                // TODO: Handle xrun
                SPDLOG_ERROR("commit {} xrun!", commitres);
            }
            size -= frames;
        }
        SPDLOG_DEBUG("ALSA PCM frames commited [engine xrun={}]", engine_xrun);
        self.callback();
    }

    return 0;
}

void AlsaPcm::process_events() {
    PcmEvent event;
    int err;
    PcmFrame_s24_3le dropped_frame;
    snd_pcm_uframes_t dropped_frames = 0;
    while (pcm_event_fifo->pop(event)) {
        SPDLOG_DEBUG("ALSA PCM event = {}", (int)event);
        switch(event) {
            case ALSA_PCM_START:
                err = snd_pcm_start(pcm_out);
                if (err < 0) {
                    SPDLOG_ERROR("snd_pcm_start failed = {}", snd_strerror(err));
                }
                break;
            case ALSA_PCM_STOP:
                err = snd_pcm_pause(pcm_out, false);
                if (err < 0) {
                    SPDLOG_ERROR("snd_pcm_pause failed = {}", snd_strerror(err));
                }
                break;
            case ALSA_PCM_CONTINUE:
                err = snd_pcm_pause(pcm_out, true);
                if (err < 0) {
                    SPDLOG_ERROR("snd_pcm_pause failed = {}", snd_strerror(err));
                }
                break;
            case ALSA_PCM_DRAIN:
                SPDLOG_DEBUG("Draining...");
                while (pcm_fifo->pop(dropped_frame)) {
                    dropped_frames++;
                }
                err = snd_pcm_reset(pcm_out);
                if (err < 0) {
                    SPDLOG_ERROR("snd_pcm_reset failed = {}", snd_strerror(err));                
                }
                SPDLOG_DEBUG("Draining done. [dropped_frames={}]", dropped_frames);
                break;
            default:
                SPDLOG_ERROR("Unknown event! [{}]", (int)event);
                break;
        }
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
    // err = snd_pcm_sw_params_set_avail_min(handle, swparams, period_event ? buffer_size : period_size);
    // if (err < 0) {
    //     SPDLOG_ERROR("Unable to set avail min for playback: {}", snd_strerror(err));
    //     return err;
    // }
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

std::unique_ptr<PcmFifo> AlsaPcm::create_pcm_fifo() {
    snd_pcm_uframes_t capacity = 1;
    while (capacity <= period_size) {
        capacity *= 2;
    }
    SPDLOG_INFO("PcmFifo [{} -> {}]", period_size, capacity);
    return std::make_unique<PcmFifo>(capacity);
}

AlsaPcm::AlsaPcm():
    pcm_out(open_pcm_out(PCM_OUT_NAME)),
    stop(false),
    pcm_fifo(create_pcm_fifo()),
    callback(0),
    pcm_event_fifo(std::make_unique<PcmEventFifo>(64)),
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

PcmFifo& AlsaPcm::get_pcm_fifo() {
    return *pcm_fifo.get();
}

void AlsaPcm::start(std::function<void(void)> _callback) {
    if (pcm_thread || callback) {
        SPDLOG_ERROR("Thread already started! [{}]", pcm_thread);
        return;
    }
    callback = _callback;
    pcm_thread = create_rt_thread("alsa_pcm", THREAD_PRIORITY, run_pcm, this);
}

void AlsaPcm::play_start() {
    snd_pcm_start(pcm_out);
    // TODO: snd_pcm_wait is waiting...
    // if (!pcm_event_fifo->push(ALSA_PCM_START)) {
    //     SPDLOG_ERROR("pcm_event_fifo overflow!");
    // }
}

void AlsaPcm::play_stop() {
    if (!pcm_event_fifo->push(ALSA_PCM_STOP)) {
        SPDLOG_ERROR("pcm_event_fifo overflow!");
    }
}

void AlsaPcm::play_continue() {
    if (!pcm_event_fifo->push(ALSA_PCM_CONTINUE)) {
        SPDLOG_ERROR("pcm_event_fifo overflow!");
    }
}

void AlsaPcm::drain() {
    if (!pcm_event_fifo->push(ALSA_PCM_DRAIN)) {
        SPDLOG_ERROR("pcm_event_fifo overflow!");
    }
}