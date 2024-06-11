#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "alsapcm.hpp"

#define SPDLOG_ACTIVE_LEVEL 2

// #define TEST_PARAMS

static constexpr std::string PCM_OUT_NAME = "hw:UMC1820";
static constexpr snd_pcm_uframes_t PCM_OUT_RATE = 96000;
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
    SPDLOG_INFO("APCM  started.");
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
        SPDLOG_INFO("APCM  stopped.");
    } catch(std::exception const& e) {
        SPDLOG_ERROR("APCM  failed [e={}]", e.what());
    }
}

void AlsaPcm::process_events(bool wait_for_event) {
    PcmEvent event;
    snd_pcm_state_t state = alsa_snd_pcm_state();
    while (pcm_event_callback(event, state, wait_for_event)) {
        wait_for_event = false;
        SPDLOG_TRACE("APCM  event = {}", (int)event);
        switch(event) {
            case ALSA_PCM_START:
                if (state != SND_PCM_STATE_PREPARED) {
                    alsa_snd_pcm_drop();
                }
                alsa_snd_pcm_start();
                break;
            case ALSA_PCM_PAUSE:
                if (state == SND_PCM_STATE_RUNNING) {
                    alsa_snd_pcm_pause();
                } else {
                    SPDLOG_WARN("APCM  invalid ALSA_PCM_PAUSE. [state={}]", (int)state);
                }
                break;
            case ALSA_PCM_RESUME:
                if (state == SND_PCM_STATE_PAUSED) {
                    alsa_snd_pcm_resume();
                } else {
                    SPDLOG_WARN("APCM  invalid ALSA_PCM_RESUME. [state={}]", (int)state);
                }
                break;
            case ALSA_PCM_PROGRAM_CHANGE:
                if (state == SND_PCM_STATE_RUNNING) {
                    SPDLOG_DEBUG("APCM  ALSA_PCM_PROGRAM_CHANGE while running.");
                    break;
                }
                alsa_snd_pcm_drop();
                break;
            default:
                SPDLOG_ERROR("APCM  unknown event! [{}]", (int)event);
                break;
        }
    }
}

bool AlsaPcm::wait_until_avail(bool& wait_for_event) {
    snd_pcm_sframes_t avail;
    wait_for_event = false;
    int err = snd_pcm_avail_delay(pcm_out, &avail, &current_delay);
    if (err < 0 || avail < 0) {
        throw OstrostrojException(fmt::format("APCM  snd_pcm_avail_delay failed = [err={},avail={}]", snd_strerror(err), avail));
    }
    snd_pcm_state_t state = alsa_snd_pcm_state();
    if (state == SND_PCM_STATE_XRUN || state == SND_PCM_STATE_SUSPENDED) {
        throw OstrostrojException(fmt::format("APCM  invalid state={}", (int)state));
    }
    if (avail < (snd_pcm_sframes_t)period_size) {
        if (state != SND_PCM_STATE_RUNNING) {
            SPDLOG_TRACE("APCM  wait_for_event. [state={}]", (int)state);
            wait_for_event = true;
        } else {
            SPDLOG_TRACE("APCM  busy loop. [state={}]", (int)state);
            usleep(std::chrono::microseconds(period_time).count());
        }
        return false;
    }
    return true;
}

void AlsaPcm::write(snd_pcm_uframes_t size) {
    const snd_pcm_channel_area_t* areas;
    snd_pcm_uframes_t offset, frames;
    PcmFrame_s24_3le* buffer;
    SPDLOG_TRACE("APCM  writing {} frames...", period_size);
    while (size > 0) {
        frames = size;
        alsa_snd_pcm_mmap_begin(&areas, &offset, &frames);
        buffer = (PcmFrame_s24_3le*)(((char*)areas[0].addr) + (areas[0].first / 8)) + offset;
        write_to_mmap(buffer, frames);
        alsa_snd_pcm_mmap_commit(offset, frames);
        size -= frames;
    }
    SPDLOG_TRACE("APCM  frames commited");
}

void AlsaPcm::write_to_mmap(PcmFrame_s24_3le* buffer, snd_pcm_uframes_t frames_to_write) {
    while (frames_to_write-- > 0) {
        pcm_callback(*buffer);
        buffer++;
    }
}

AlsaPcm::AlsaPcm():
    pcm_out(open_pcm_out(PCM_OUT_NAME)),
    stop(false),
    pcm_event_callback(0),
    pcm_callback(0),
    pcm_thread(0){    
}

AlsaPcm::~AlsaPcm() {
    shutdown();
    if (pcm_out) {
        snd_pcm_close(pcm_out);
        pcm_out = nullptr;
    }
}

snd_pcm_uframes_t AlsaPcm::get_sample_rate() const {
    return PCM_OUT_RATE;
}

int AlsaPcm::get_channels() const {
    return PCM_OUT_CHANNELS;    
}

std::chrono::milliseconds AlsaPcm::get_period_time() {
    return period_time;
}

snd_pcm_uframes_t AlsaPcm::get_period_size() {
    return period_size;
}

uint AlsaPcm::get_periods() {
    return periods;
}

void AlsaPcm::start(std::function<bool(PcmEvent&, snd_pcm_state_t, bool)> _pcm_event_callback, std::function<void(PcmFrame_s24_3le&)> _pcm_callback) {
    if (pcm_thread || pcm_callback) {
        SPDLOG_ERROR("APCM  thread already started! [{}]", pcm_thread);
        return;
    }
    pcm_event_callback = _pcm_event_callback;
    pcm_callback = _pcm_callback;
    pcm_thread = create_rt_thread("alsa_pcm", THREAD_PRIORITY, run_pcm, this);
}

void AlsaPcm::shutdown() {
    if (pcm_thread) {
        stop = true;
        void* status;
        pthread_join(pcm_thread, &status);
        pcm_thread = 0;
    }
}