#include "common.hpp"
#include <alsa/asoundlib.h>
#include "alsamidi.hpp"

void AlsaMidi::run_thru() {
    while (!stop) {
        unsigned char ch;
        snd_rawmidi_read(handle_in, &ch, 1);
        SPDLOG_TRACE("thru: 0x{:x}", ch);        
        snd_rawmidi_write(handle_out, &ch, 1);
        snd_rawmidi_drain(handle_out);
    }
}

snd_rawmidi_t* AlsaMidi::open(const std::string& device_name) {
    snd_rawmidi_t* result;
    int err;
    err = snd_rawmidi_open(&result, NULL, device_name.c_str(), 0);    
    if (err) {
        SPDLOG_ERROR("snd_rawmidi_open {} failed: {}", device_name, err);
    }
    return result;
}

pthread_t AlsaMidi::create_rt_thread() {
    // https://github.com/jackaudio/jack2/blob/c46c1b16e0eabbcf55ef69b0ffb96dfe16521cfa/posix/JackPosixThread.cpp#L117
    pthread_attr_t attributes;
    pthread_attr_init(&attributes);
    int res;
    if ((res = pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_JOINABLE))) {
        SPDLOG_ERROR("Cannot request joinable thread creation for thread res = {}", res);
        return;
    }
    if ((res = pthread_attr_setscope(&attributes, PTHREAD_SCOPE_SYSTEM))) {
        SPDLOG_ERROR("Cannot set scheduling scope for thread res = {}", res);
        return;
    }
    if ((res = pthread_attr_setinheritsched(&attributes, PTHREAD_EXPLICIT_SCHED))) {
        SPDLOG_ERROR("Cannot request explicit scheduling for RT thread res = {}", res);
        return;
    }
    if ((res = pthread_attr_setschedpolicy(&attributes, SCHED_FIFO))) {
        SPDLOG_ERROR("Cannot set RR scheduling class for RT thread res = {}", res);
        return;
    }
    struct sched_param rt_param;
    memset(&rt_param, 0, sizeof(rt_param));
    rt_param.sched_priority = -10;
    if ((res = pthread_attr_setschedparam(&attributes, &rt_param))) {
        SPDLOG_ERROR("Cannot set scheduling priority for RT thread res = {}", res);
        return;
    }
    if ((res = pthread_attr_setstacksize(&attributes, 524288))) {
        SPDLOG_ERROR("Cannot set thread stack size res = {}", res);
        return;
    }
    pthread_t result;
    if ((res = pthread_create(&result, &attributes, start_routine, arg))) {
        SPDLOG_ERROR("Cannot create thread res = {}", res);
        return;
    }
    pthread_attr_destroy(&attributes);
    return result;
}

AlsaMidi::AlsaMidi():
    handle_in(open(device_in)),
    handle_out(open(device_out)),
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