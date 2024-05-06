#include "common.hpp"
#include <alsa/asoundlib.h>
#include "alsamidi.hpp"

// TODO: https://alsamodular.sourceforge.net/seqdemo.c

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
                    bool pass = true;
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
                            pass = ((event.data.control.value % 2) == 0);
                            break;
                        default:
                            SPDLOG_TRACE("thru: 0x{:x}", ch);
                            break;
                    }
                    if (pass) {
                        snd_rawmidi_write(self.handle_out, decoded_current, event_encode_res);
                        snd_rawmidi_drain(self.handle_out);
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
    thru_thread(create_rt_thread(80, run_thru, this)) {
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