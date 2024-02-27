#include <string>
#include <format>
#include <spdlog/spdlog.h>
#include <jack/midiport.h>
#include "soundcard.h"
#include "common.h"

SoundCard::SoundCard(const std::string &name) :
    jack_client(create_client(name, this)),
    midi_input_port(create_midi_input_port(jack_client)) {
    registerCallbacks();
    activate();
    connect();
}

SoundCard::~SoundCard() {
    if (jack_client != nullptr) {
        jack_port_unregister(jack_client, midi_input_port);
        jack_client_close(jack_client);
        spdlog::info("Jack client closed.");
    }
}

int SoundCard::process_callback(jack_nframes_t nframes, void *arg) {
    const SoundCard *soundCard = static_cast<SoundCard*>(arg);

    void* port_buf = jack_port_get_buffer(soundCard->midi_input_port, nframes);
	jack_nframes_t event_count = jack_midi_get_event_count(port_buf);
    jack_midi_event_t in_event;
    spdlog::debug(std::format("{} MIDI events", event_count));
    for(unsigned int i = 0; i < event_count; i++) {
        auto result = jack_midi_event_get(&in_event, port_buf, i);
        if (result != 0) {
            spdlog::error(std::format("MIDI event get failed! [{}]", result));
        } else {
            // spdlog::debug(std::format("MIDI event="))
        }
    }
    return 0;
}

void SoundCard::port_connect_callback(jack_port_id_t a, jack_port_id_t b, int connect, void *arg) {
    const SoundCard *soundCard = static_cast<SoundCard*>(arg);
    auto a_name = jack_port_name(jack_port_by_id(soundCard->jack_client, a));
    auto b_name = jack_port_name(jack_port_by_id(soundCard->jack_client, b));
    if (connect == 0) {
        spdlog::info(std::format("Port {} disconnected from {}.", a_name, b_name));
    } else {
        spdlog::info(std::format("Port {} connected to {}.", a_name, b_name));
    }
}

void SoundCard::port_registration_callback(jack_port_id_t port, int registered, void *arg) {
    const SoundCard *soundCard = static_cast<SoundCard*>(arg);
    auto port_name = jack_port_name(jack_port_by_id(soundCard->jack_client, port));
    if (registered == 0) {
        spdlog::info(std::format("Port {} unregistered.", port_name));
    } else {
        spdlog::info(std::format("Port {} registered.", port_name));
    }
}

jack_client_t * SoundCard::create_client(const std::string &name, SoundCard *) {
    jack_status_t status;
    auto jack_client = jack_client_open(name.c_str(), JackNoStartServer, &status);
    if (nullptr == jack_client) {        
        throw OstrostrojException(std::format("Jack client open failed! [status=0x{:x}]", static_cast<int>(status)));
    }
    spdlog::info(std::format("Jack client open. [status=0x{:x}]", static_cast<int>(status)));
    return jack_client;
}

jack_port_t * SoundCard::create_midi_input_port(jack_client_t * jack_client) {
    jack_port_t * const result = jack_port_register(jack_client, "midi_input", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
    if (result == nullptr) {
        throw OstrostrojException("Jack MIDI port registration failed!");
    }
    return result;
}

void SoundCard::libremidi_message_callback(int port, libremidi::message& message) {
    std::string log_message;
    switch (message.get_message_type()) {        
        case libremidi::message_type::NOTE_ON:
            log_message = std::format("NOTE_ON {}", (int)message.bytes[1]);
            break;
        case libremidi::message_type::PROGRAM_CHANGE:
            log_message = std::format("PROGRAM_CHANGE {}", (int)message.bytes[1]);
            break;
        case libremidi::message_type::START:
            log_message = "START";
            break;
        case libremidi::message_type::CONTINUE:
            log_message = "CONTINUE";
            break;            
        case libremidi::message_type::STOP:
            log_message = "STOP";
            break;
        case libremidi::message_type::TIME_CLOCK:
            log_message = std::format("TIME_CLOCK {}", (int)message.bytes[1]);
            break;
        default:
            log_message = std::format("{}", static_cast<int>(message.get_message_type()));
            break;
    }
    spdlog::info(std::format("{} [port{}, ch{}]", log_message, port, message.get_channel()));
}

void SoundCard::registerCallbacks() {
    const auto set_callback_result = jack_set_process_callback(jack_client, process_callback, this);
    if (set_callback_result != 0) {
        throw OstrostrojException(std::format("Jack set process callback failed! [status=0x{:x}]", set_callback_result));        
    }
    const auto set_port_connect_callback_result = jack_set_port_connect_callback(jack_client, port_connect_callback, this);
    if (set_port_connect_callback_result != 0) {
        throw OstrostrojException(std::format("Jack set port connect callback failed! [status=0x{:x}]", set_port_connect_callback_result));        
    }
    const auto jack_set_port_registration_callback_result = jack_set_port_registration_callback(jack_client, port_registration_callback, this);
    if (jack_set_port_registration_callback_result != 0) {
        throw OstrostrojException(std::format("Jack set port registration callback failed! [status=0x{:x}]", jack_set_port_registration_callback_result));        
    }
}

void SoundCard::activate() {
    const auto activate_result = jack_activate(jack_client);
    if (activate_result != 0) {
        throw OstrostrojException(std::format("Jack activate failed! [status=0x{:x}]", activate_result));        
    }
    spdlog::info("Jack client activated.");
}

void SoundCard::connect() {
    const auto connect_result = jack_connect(jack_client, MIDI_INPUT_PORT.c_str(), jack_port_name(midi_input_port));
    if (connect_result != 0) {
        throw OstrostrojException(std::format("Jack connect failed! [status=0x{:x}]", connect_result));        
    }
    spdlog::info("MIDI ports connected.");
}