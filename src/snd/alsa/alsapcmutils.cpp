#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "alsapcm.hpp"

static constexpr snd_pcm_access_t PCM_OUT_ACCESS = SND_PCM_ACCESS_MMAP_INTERLEAVED;
static constexpr snd_pcm_format_t PCM_OUT_FORMAT = SND_PCM_FORMAT_S24_3LE;
static constexpr std::chrono::duration<long, std::milli> PCM_OUT_PERIOD_TIME = std::chrono::milliseconds(5);
static constexpr int PCM_OUT_BUFFER_PERIODS = 8;

void AlsaPcm::alsa_snd_pcm_mmap_begin(const snd_pcm_channel_area_t **areas, snd_pcm_uframes_t *offset, snd_pcm_uframes_t *frames) {
    int err = snd_pcm_mmap_begin(pcm_out, areas, offset, frames);
    SPDLOG_TRACE("APCM  snd_pcm_mmap_begin = {}, {}, {}", err, *offset, *frames);
    if (err < 0) {
        throw OstrostrojException(fmt::format("APCM  snd_pcm_mmap_begin failed={}", snd_strerror(err)));
    }
}

void AlsaPcm::alsa_snd_pcm_mmap_commit(snd_pcm_uframes_t offset, snd_pcm_uframes_t frames) {
    snd_pcm_sframes_t commitres = snd_pcm_mmap_commit(pcm_out, offset, frames);
    SPDLOG_TRACE("APCM  snd_pcm_mmap_commit = {}, {}, {}", commitres, offset, frames);       
    if (commitres < 0 || (snd_pcm_uframes_t)commitres != frames) {
        throw OstrostrojException(fmt::format("APCM  snd_pcm_mmap_commit failed={}", commitres));
    }
}

snd_pcm_state_t AlsaPcm::alsa_snd_pcm_state() {
    snd_pcm_state_t result = snd_pcm_state(pcm_out);
    SPDLOG_TRACE("APCM  snd_pcm_state = {}", (long)result);
    if (result < 0) {
        throw OstrostrojException(fmt::format("APCM  snd_pcm_state failed={}", snd_strerror(result)));
    }
    return result;
}

void AlsaPcm::alsa_snd_pcm_start() {
    SPDLOG_DEBUG("APCM  alsa_snd_pcm_start");
    int err = snd_pcm_start(pcm_out);
    if (err < 0) {
        throw OstrostrojException(fmt::format("APCM  snd_pcm_start failed={}", snd_strerror(err)));
    }
}

void AlsaPcm::alsa_snd_pcm_pause() {    
    SPDLOG_DEBUG("APCM  alsa_snd_pcm_pause");
    int err = snd_pcm_pause(pcm_out, true);
    if (err < 0) {
        throw OstrostrojException(fmt::format("APCM  snd_pcm_pause (pause) failed={}", snd_strerror(err)));
    }
}

void AlsaPcm::alsa_snd_pcm_resume() {
    SPDLOG_DEBUG("APCM  alsa_snd_pcm_resume");
    int err = snd_pcm_pause(pcm_out, false);
    if (err < 0) {
        throw OstrostrojException(fmt::format("APCM  snd_pcm_pause (resume) failed={}", snd_strerror(err)));
    }  
}

void AlsaPcm::alsa_snd_pcm_drop() {
    SPDLOG_DEBUG("APCM  alsa_snd_pcm_drop");
    int err = snd_pcm_drop(pcm_out);
    if (err < 0) {
        throw OstrostrojException(fmt::format("APCM  snd_pcm_drop failed={}", snd_strerror(err)));
    }
    err = snd_pcm_prepare(pcm_out);
    if (err < 0) {
        throw OstrostrojException(fmt::format("APCM  snd_pcm_prepare failed={}", snd_strerror(err)));
    }
    snd_pcm_state_t state = alsa_snd_pcm_state();
    if (state != SND_PCM_STATE_PREPARED) {
        throw OstrostrojException(fmt::format("APCM  Not in SND_PCM_STATE_PREPARED state. [{}]", (int)state));        
    }
}
 
int AlsaPcm::set_hwparams(snd_pcm_t* handle, snd_pcm_hw_params_t* params) {
    unsigned int rrate;
    snd_pcm_uframes_t size;
    int err, dir;
    err = snd_pcm_hw_params_any(handle, params);
    if (err < 0) {
        throw OstrostrojException(fmt::format("APCM  broken configuration for playback: no configurations available: {}", snd_strerror(err)));
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
        throw OstrostrojException(fmt::format("APCM  access type not available for playback: {}", snd_strerror(err)));
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
        throw OstrostrojException(fmt::format("APCM  sample format not available for playback: {}", snd_strerror(err)));
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
        throw OstrostrojException(fmt::format("APCM  channels count ({}) not available for playbacks: {}", PCM_OUT_CHANNELS, snd_strerror(err)));
    }
    rrate = get_sample_rate();
    err = snd_pcm_hw_params_set_rate_near(handle, params, &rrate, 0);
    if (err < 0) {
        throw OstrostrojException(fmt::format("APCM  rate {}Hz not available for playback: {}", get_sample_rate(), snd_strerror(err)));
    }
    if (rrate != get_sample_rate()) {
        throw OstrostrojException(fmt::format("APCM  rate doesn't match (requested {}Hz, get {}Hz)", get_sample_rate(), err));
    }
    size = PCM_OUT_PERIOD_TIME.count() * get_sample_rate() / 1000;
    period_size = 1;
    while (period_size < size) {
        period_size *= 2;
    }
    err = snd_pcm_hw_params_set_period_size(handle, params, period_size, 0);
    if (err < 0) {
        snd_pcm_uframes_t min, max;
        snd_pcm_hw_params_get_period_size_min(params, &min, &dir);
        snd_pcm_hw_params_get_period_size_max(params, &max, &dir);

        throw OstrostrojException(fmt::format("APCM  snd_pcm_hw_params_set_period_size failed = {} [period_size={} <{}:{}>]", snd_strerror(err), period_size, min, max));
    }
    err = snd_pcm_hw_params_get_period_size(params, &period_size, &dir);
    if (err < 0) {
        throw OstrostrojException(fmt::format("APCM  snd_pcm_hw_params_get_period_size failed = {}", snd_strerror(err)));
    }
    period_time = std::chrono::milliseconds((1000 * period_size) / get_sample_rate());

    err = snd_pcm_hw_params_set_buffer_size(handle, params, period_size * PCM_OUT_BUFFER_PERIODS);
    if (err < 0) {
        throw OstrostrojException(fmt::format("APCM  snd_pcm_hw_params_set_buffer_size failed = {}", snd_strerror(err)));
    }
    err = snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
    if (err < 0) {
        throw OstrostrojException(fmt::format("APCM  snd_pcm_hw_params_get_buffer_size failed = {}", snd_strerror(err)));
    }
    err = snd_pcm_hw_params_get_periods(params, &periods, &dir);
    if (err < 0) {
        throw OstrostrojException(fmt::format("APCM  snd_pcm_hw_params_get_periods failed = {}", snd_strerror(err)));
    }
    err = snd_pcm_hw_params_can_pause(params);
    if (err < 0) {
        throw OstrostrojException(fmt::format("APCM  snd_pcm_hw_params_can_pause failed = {}", snd_strerror(err)));
    }
    err = snd_pcm_hw_params(handle, params);
    if (err < 0) {
        throw OstrostrojException(fmt::format("APCM  Unable to set hw params for playback: {}", snd_strerror(err)));
    }
    return 0;
}
 
int AlsaPcm::set_swparams(snd_pcm_t* handle, snd_pcm_sw_params_t* swparams) {
    int err; 
    // get the current swparams
    err = snd_pcm_sw_params_current(handle, swparams);
    if (err < 0) {
        throw OstrostrojException(fmt::format("APCM  unable to determine current swparams for playback: {}", snd_strerror(err)));
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
    err = snd_pcm_sw_params_set_avail_min(handle, swparams, 512);
    if (err < 0) {
        throw OstrostrojException(fmt::format("APCM  snd_pcm_sw_params_set_avail_min failed = {}", snd_strerror(err)));
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
        throw OstrostrojException(fmt::format("APCM  unable to set sw params for playback: {}", snd_strerror(err)));
    }
    return 0;
}

void AlsaPcm::wait_for_device(const std::string& pcm_out_name) {
    static constexpr std::chrono::milliseconds WAIT = std::chrono::milliseconds(500);
    SPDLOG_INFO("APCM  waiting for device...");
    while (true) {
        int card = -1;
        do {
            int err = snd_card_next(&card);
            if (err) {
                throw OstrostrojException(fmt::format("APCM  snd_card_next failed = {}", snd_strerror(err)));
            }
            if (card > -1) {
                char* card_name;
                err = snd_card_get_name(card, &card_name);
                if (err) {
                    throw OstrostrojException(fmt::format("APCM  snd_card_get_name failed = {}", snd_strerror(err)));
                }
                SPDLOG_INFO("APCM  card_name={}", card_name);
                if ("hw:" + std::string(card_name) == pcm_out_name) {
                    return;
                }
            }
        } while (card > -1);
        usleep(std::chrono::microseconds(WAIT).count());
    }
}

snd_pcm_t* AlsaPcm::open_pcm_out(const std::string& pcm_out_name) {
    wait_for_device(pcm_out_name);
    snd_pcm_t* result;
    int err = snd_pcm_open(&result, pcm_out_name.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
    if (err) {
        throw OstrostrojException(fmt::format("APCM  snd_pcm_open failed = {}", snd_strerror(err)));
    }
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_hw_params_alloca(&hwparams);
    err = set_hwparams(result, hwparams);
    if (err < 0) {
        throw OstrostrojException(fmt::format("APCM  setting of hwparams failed: {}", snd_strerror(err)));
    }
    snd_pcm_sw_params_t *swparams;
    snd_pcm_sw_params_alloca(&swparams);
    err = set_swparams(result, swparams);
    if (err < 0) {
        throw OstrostrojException(fmt::format("APCM  setting of swparams failed: {}", snd_strerror(err)));
    }
    const char* pcm_name = snd_pcm_name(result);
    snd_pcm_type_t pcm_type = snd_pcm_type(result);
    snd_pcm_sframes_t delay;
    snd_pcm_delay(result, &delay);
    snd_pcm_uframes_t start_threshold;
    snd_pcm_sw_params_get_start_threshold(swparams, &start_threshold);
    snd_pcm_uframes_t avail_min;
    snd_pcm_sw_params_get_avail_min(swparams, &avail_min);
    SPDLOG_INFO("APCM  open. [name={},buffer_size={},period_size={},period_time={}ms,periods={},type={},delay={},start_threshold={},avail_min={},sample={}bytes]",
        pcm_name, buffer_size, period_size, period_time.count(), periods, (int)pcm_type, delay, start_threshold, avail_min, sizeof(PcmSample_s24_3le));
    return result;
}