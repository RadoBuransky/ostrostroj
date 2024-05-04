#include "common.hpp"
#include "alsaseqmidi.hpp"

// https://alsamodular.sourceforge.net/seqdemo.c
// https://tldp.org/HOWTO/MIDI-HOWTO-9.html

snd_seq_t* AlsaSeqMidi::open_client() {
    snd_seq_t *handle;
    int err;
    err = snd_seq_open(&handle, "default", SND_SEQ_OPEN_INPUT, 0);
    if (err < 0) {
        SPDLOG_ERROR("snd_seq_open failed = {}", err);
        return nullptr;
    }
    snd_seq_set_client_name(handle, "Ostrostroj MIDI");
    return handle;
}

int AlsaSeqMidi::create_input_port() {  
    return snd_seq_create_simple_port(client, "Ostrostroj input",
            SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
            SND_SEQ_PORT_TYPE_MIDI_GENERIC|SND_SEQ_PORT_TYPE_SOFTWARE|SND_SEQ_PORT_TYPE_APPLICATION);
}

int AlsaSeqMidi::create_output_port() {  
    return snd_seq_create_simple_port(client, "Ostrostroj output",
            SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
            SND_SEQ_PORT_TYPE_MIDI_GENERIC|SND_SEQ_PORT_TYPE_SOFTWARE|SND_SEQ_PORT_TYPE_APPLICATION);
}

void AlsaSeqMidi::midi_action() {
  snd_seq_event_t *ev;
  do {
    snd_seq_event_input(client, &ev);
    switch (ev->type) {
        case SND_SEQ_EVENT_START:
            SPDLOG_INFO("SND_SEQ_EVENT_START() [{}]", ev->data.control.channel);
            break;
        case SND_SEQ_EVENT_STOP:
            SPDLOG_INFO("SND_SEQ_EVENT_STOP() [{}]", ev->data.control.channel);
            break;
        case SND_SEQ_EVENT_CONTINUE:
            SPDLOG_INFO("SND_SEQ_EVENT_CONTINUE() [{}]", ev->data.control.channel);
            break;
        default:
            SPDLOG_INFO("{}", ev->type);
            break;
    }
    snd_seq_free_event(ev);
  } while (snd_seq_event_input_pending(client, 0) > 0);
}

AlsaSeqMidi::AlsaSeqMidi():
    client(open_client()),
    in_port(create_input_port()),
    out_port(create_output_port()) {
    SPDLOG_INFO("ALSA MIDI sequencer open. [in={}, out={}]", in_port, out_port);
    // std::vector<pollfd> pds;
    // pds.reserve(snd_seq_poll_descriptors_count(client, POLLIN));
    // int err = snd_seq_poll_descriptors(client, pds.data(), pds.capacity(), POLLIN);
    // if (err < pds.capacity()) {
    //     SPDLOG_ERROR("snd_seq_poll_descriptors failed = {}", err);
    //     return;
    // }
    // SPDLOG_INFO("ALSA MIDI sequencer polling... [{}]", pds.capacity());
    // while (1) {
    //     if (poll(pds.data(), pds.capacity(), 100000) > 0) {
    //         midi_action();
    //     }
    // }
    snd_seq_event_t* ev;
    while (1) {
        int res = snd_seq_event_input(client, &ev);
        if (res < 0) {
            SPDLOG_ERROR("snd_seq_event_input error = {}", res);
        } else {
            SPDLOG_INFO("Event type = {}", ev->type);
        }
    }
}

AlsaSeqMidi::~AlsaSeqMidi() {
    if (client != nullptr) {
        snd_seq_close(client);
        client = nullptr;
        SPDLOG_INFO("ALSA MIDI sequencer closed.");
    }
}