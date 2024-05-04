#include "common.hpp"
#include <alsa/asoundlib.h>
#include "alsamidi.hpp"

// TODO: https://alsamodular.sourceforge.net/seqdemo.c

static void* run_thru(void* context) {
    AlsaMidi& self = *(AlsaMidi*)context;
    snd_midi_event_t* parser;
    int res = snd_midi_event_new(64, &parser);
    if (res < 0) {
        SPDLOG_ERROR("snd_midi_event_new failed = {}", res);
        return 0;
    }
    snd_seq_event_t event;
    std::array<unsigned char, 64> decoded;
    SPDLOG_INFO("ALSA rawmidi thru started.");
    while (!self.stop) {
        unsigned char ch;
        snd_rawmidi_read(self.handle_in, &ch, 1);
        res = snd_midi_event_encode_byte(parser, ch, &event);
        if (res < 0) {
            SPDLOG_ERROR("snd_midi_event_encode_byte failed = {}", res);
            snd_midi_event_reset_encode(parser);
        } else {
            if (res == 1) {
                switch (event.type) {
                    case SND_SEQ_EVENT_START:
                        SPDLOG_INFO("SND_SEQ_EVENT_START");
                        break;
                    case SND_SEQ_EVENT_CONTINUE:
                        SPDLOG_INFO("SND_SEQ_EVENT_CONTINUE");
                        break;
                    case SND_SEQ_EVENT_STOP:
                        SPDLOG_INFO("SND_SEQ_EVENT_STOP");
                        break;
                    case SND_SEQ_EVENT_PGMCHANGE:                        
                        SPDLOG_INFO("SND_SEQ_EVENT_PGMCHANGE [ch={},param={},value={}]", event.data.control.channel,
                            event.data.control.param, event.data.control.value);
                        break;
                    default:
                        SPDLOG_TRACE("thru: 0x{:x}", ch);
                        break;
                }
                int decoded_size = snd_midi_event_decode(parser, decoded.data(), decoded.size(), &event);
                if (decoded_size <= 0) {
                    SPDLOG_ERROR("snd_midi_event_decode failed = {}", decoded_size);
                } else {
                    snd_rawmidi_write(self.handle_out, decoded.data(), decoded_size);
                    snd_rawmidi_drain(self.handle_out);
                }
            }
        }
    }
    SPDLOG_INFO("ALSA rawmidi thru stopped.");
    snd_midi_event_free(parser);
    return 0;
}

snd_rawmidi_t* AlsaMidi::open_in(const std::string& device_name) {
    snd_rawmidi_t* result;
    int err;
    err = snd_rawmidi_open(&result, NULL, device_name.c_str(), 0);    
    if (err) {
        SPDLOG_ERROR("snd_rawmidi_open {} failed: {}", device_name, err);
    }
    snd_rawmidi_params_t *params;
    snd_rawmidi_params_malloc(&params);
    snd_rawmidi_params_current(result, params);
    size_t old_buffer_size = snd_rawmidi_params_get_buffer_size(params);
    SPDLOG_INFO("Old buffer size = {}", old_buffer_size);
    // snd_rawmidi_params_set_buffer_size(result, params, 64*1024);
    err = snd_rawmidi_params(result, params);  
    if (err) {
        SPDLOG_ERROR("snd_rawmidi_params {} failed: {}", device_name, err);
    }
    snd_rawmidi_params_free(params);
    SPDLOG_INFO("ALSA rawmidi input open. [{}]", device_name);
    return result;
}

snd_rawmidi_t* AlsaMidi::open_out(const std::string& device_name) {
    snd_rawmidi_t* result;
    int err;
    err = snd_rawmidi_open(NULL, &result, device_name.c_str(), 0);    
    if (err) {
        SPDLOG_ERROR("snd_rawmidi_open {} failed: {}", device_name, err);
    }
    SPDLOG_INFO("ALSA rawmidi output open. [{}]", device_name);
    return result;
}

pthread_t AlsaMidi::create_rt_thread() {
    // https://github.com/jackaudio/jack2/blob/c46c1b16e0eabbcf55ef69b0ffb96dfe16521cfa/posix/JackPosixThread.cpp#L117
    pthread_attr_t attributes;
    pthread_attr_init(&attributes);
    int res;
    if ((res = pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_JOINABLE))) {
        SPDLOG_ERROR("Cannot request joinable thread creation for thread res = {}", res);
        return 0;
    }
    if ((res = pthread_attr_setscope(&attributes, PTHREAD_SCOPE_SYSTEM))) {
        SPDLOG_ERROR("Cannot set scheduling scope for thread res = {}", res);
        return 0;
    }
    if ((res = pthread_attr_setinheritsched(&attributes, PTHREAD_EXPLICIT_SCHED))) {
        SPDLOG_ERROR("Cannot request explicit scheduling for RT thread res = {}", res);
        return 0;
    }
    if ((res = pthread_attr_setschedpolicy(&attributes, SCHED_FIFO))) {
        SPDLOG_ERROR("Cannot set RR scheduling class for RT thread res = {}", res);
        return 0;
    }
    struct sched_param rt_param;
    memset(&rt_param, 0, sizeof(rt_param));
    rt_param.sched_priority = 90;
    if ((res = pthread_attr_setschedparam(&attributes, &rt_param))) {
        SPDLOG_ERROR("Cannot set scheduling priority for RT thread res = {}", res);
        return 0;
    }
    if ((res = pthread_attr_setstacksize(&attributes, 524288))) {
        SPDLOG_ERROR("Cannot set thread stack size res = {}", res);
        return 0;
    }
    pthread_t result;
    if ((res = pthread_create(&result, &attributes, run_thru, this))) {
        SPDLOG_ERROR("Cannot create thread res = {}", res);
        return 0;
    }
    pthread_attr_destroy(&attributes);
    SPDLOG_INFO("ALSA thread created. [0x{:X}]", result);
    return result;
}

AlsaMidi::AlsaMidi():
    handle_in(open_in(device)),
    handle_out(open_out(device)),
    stop(false),
    thru_thread(create_rt_thread()) {
}

AlsaMidi::~AlsaMidi() {
    stop = true;
    void* status;
    pthread_join(thru_thread, &status);
    if (handle_in) {
        snd_rawmidi_drain(handle_in);
        snd_rawmidi_close(handle_in);
    }
    if (handle_out) {
        snd_rawmidi_drain(handle_out);
        snd_rawmidi_close(handle_out);
    }
}