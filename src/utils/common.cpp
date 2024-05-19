#include "common.hpp"

pthread_t create_rt_thread(std::string name, int sched_priority, void *(*start_routine) (void *), void* arg) {
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
    rt_param.sched_priority = sched_priority;
    if ((res = pthread_attr_setschedparam(&attributes, &rt_param))) {
        SPDLOG_ERROR("Cannot set scheduling priority for RT thread res = {}", res);
        return 0;
    }
    if ((res = pthread_attr_setstacksize(&attributes, 524288))) {
        SPDLOG_ERROR("Cannot set thread stack size res = {}", res);
        return 0;
    }
    pthread_t result;
    if ((res = pthread_create(&result, &attributes, start_routine, arg))) {
        SPDLOG_ERROR("Cannot create thread res = {}", res);
        return 0;
    }
    pthread_attr_destroy(&attributes);
    if ((res = pthread_setname_np(result, name.c_str()))) {
        SPDLOG_ERROR("pthread_setname_np failed = {}", res);
    }
    SPDLOG_DEBUG("ALSA thread created. [name={},0x{:X}]", name, result);
    return result;
}