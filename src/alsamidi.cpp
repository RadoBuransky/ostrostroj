#include "common.hpp"
#include <alsa/asoundlib.h>
#include "alsamidi.hpp"

void* run_thru(void* context) {
    AlsaMidi& self = *(AlsaMidi*)context;
    snd_midi_event_t* parser;
    int res = snd_midi_event_new(256, &parser);
    if (res < 0) {
        SPDLOG_ERROR("snd_midi_event_new failed = {}", res);
        return 0;
    }
    snd_seq_event_t event;
    std::array<unsigned char, 64> decoded;
    unsigned char* decoded_current;
    bool pass;
    bool push;
    SPDLOG_INFO("ALSA rawmidi thru started.");

    while (!self.stop) {
        decoded_current = decoded.data();
        ssize_t read_size = snd_rawmidi_read(self.handle_in, decoded.data(), decoded.size());
        while (read_size > 0) {
            int event_encode_res = snd_midi_event_encode(parser, decoded_current, read_size, &event);
            if (event_encode_res < 0) {
                SPDLOG_ERROR("snd_midi_event_encode_byte failed = {}", event_encode_res);
                snd_midi_event_reset_encode(parser);
                decoded_current = decoded.data();
                read_size = 0;
            } else {
                if (event.type != SND_SEQ_EVENT_NONE) {
#ifndef NDEBUG
                    if (event.type != SND_SEQ_EVENT_CLOCK) {
                        SPDLOG_INFO("ALSA MIDI event [type={}]", (int)event.type);
                    }
#endif
                    pass = true;
                    push = false;
                    switch (event.type) {
                        case SND_SEQ_EVENT_START:
                        case SND_SEQ_EVENT_CONTINUE:
                        case SND_SEQ_EVENT_STOP:
                        case SND_SEQ_EVENT_SETPOS_TICK:
                        case SND_SEQ_EVENT_SETPOS_TIME:
                        case SND_SEQ_EVENT_PGMCHANGE:
                            push = true;
                            break;
                    }
                    if (pass) {
                        snd_rawmidi_write(self.handle_out, decoded_current, event_encode_res);
                        snd_rawmidi_drain(self.handle_out);
                    }
                    if (push) {
                        if (!self.fifo.push(std::move(event))) {
                            SPDLOG_ERROR("MIDI FIFO overrun!");
                        }
                        self.callback();
                    }
                }
                decoded_current += event_encode_res;
                read_size -= event_encode_res;
                if (read_size > 0 && decoded_current >= decoded.end()) {
                    SPDLOG_ERROR("Decoded buffer overflow!");
                    decoded_current = decoded.data();
                }
            }
        }
    }
    SPDLOG_INFO("ALSA rawmidi thru stopped.");
    snd_midi_event_free(parser);
    return 0;
}

snd_rawmidi_t* AlsaMidi::open_midi_in(const std::string& device_name) {
    snd_rawmidi_t* result;
    int err;
    err = snd_rawmidi_open(&result, NULL, device_name.c_str(), 0);    
    if (err) {
        SPDLOG_ERROR("snd_rawmidi_open {} failed: {}", device_name, err);
    }
    snd_rawmidi_params_t *params;
    snd_rawmidi_params_malloc(&params);
    snd_rawmidi_params_current(result, params);
    size_t avail_min = snd_rawmidi_params_get_avail_min(params);
    SPDLOG_INFO("ALSA rawmidi params [avail_min={}]", avail_min);
    // err = snd_rawmidi_params(result, params);  
    // if (err) {
    //     SPDLOG_ERROR("snd_rawmidi_params {} failed: {}", device_name, err);
    // }
    snd_rawmidi_params_free(params);
    SPDLOG_INFO("ALSA rawmidi input open. [{}]", device_name);
    return result;
}

snd_rawmidi_t* AlsaMidi::open_midi_out(const std::string& device_name) {
    snd_rawmidi_t* result;
    int err;
    err = snd_rawmidi_open(NULL, &result, device_name.c_str(), 0);    
    if (err) {
        SPDLOG_ERROR("snd_rawmidi_open {} failed: {}", device_name, err);
    }
    SPDLOG_INFO("ALSA rawmidi output open. [{}]", device_name);
    return result;
}

AlsaMidi::AlsaMidi():
    handle_in(open_midi_in(MIDI_DEVICE_NAME)),
    handle_out(open_midi_out(MIDI_DEVICE_NAME)),
    stop(false),
    fifo(AlsaMidiFifo(256)),
    thru_thread(0),
    callback(0) {
}

AlsaMidi::~AlsaMidi() {
    if (thru_thread) {
        stop = true;
        void* status;
        pthread_join(thru_thread, &status);
    }
    if (handle_in) {
        snd_rawmidi_drain(handle_in);
        snd_rawmidi_close(handle_in);
    }
    if (handle_out) {
        snd_rawmidi_drain(handle_out);
        snd_rawmidi_close(handle_out);
    }
}

AlsaMidiFifo& AlsaMidi::get_fifo() {
    return fifo;
}

void AlsaMidi::start(std::function<void(void)> _callback) {
    if (thru_thread || callback) {
        SPDLOG_ERROR("Thread already started! [{}]", thru_thread);
        return;
    }
    callback = _callback;
    thru_thread = create_rt_thread("alsa_midi", 80, run_thru, this);
}