#include <string>
#include <format>
#include "soundcard.h"
#include "common.h"
#include "spdlog/spdlog.h"

static int inprocess(jack_nframes_t nframes, void *arg) {
    const SoundCard *soundCard = static_cast<SoundCard*>(arg);
    return 0;
}

static jack_client_t * create_client(const std::string &name, SoundCard *soundCard) {
    jack_status_t status;
    auto jack_client = jack_client_open(name.c_str(), JackNoStartServer, &status);
    if (nullptr == jack_client) {        
        throw OstrostrojException(std::format("Jack client open failed! [status=0x{:x}]", static_cast<int>(status)));
    }
    spdlog::info(std::format("Jack client open. [status=0x{:x}]", static_cast<int>(status)));
    const auto set_callback_result = jack_set_process_callback(jack_client, inprocess, soundCard);
    if (set_callback_result != 0) {
        throw OstrostrojException(std::format("Jack set process callback failed! [status=0x{:x}]", set_callback_result));        
    }
    return jack_client;
};

static jack_port_t * create_midi_input_port(jack_client_t * jack_client) {
    jack_port_t * const result = jack_port_register(jack_client, "midi_input", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
    if (result == nullptr) {
        throw OstrostrojException("Jack MIDI port registration failed!");
    }
    return result;
}

SoundCard::SoundCard(const std::string &name) :
    jack_client(create_client(name, this)),
    midi_input_port(create_midi_input_port(jack_client)) {
    const auto activate_result = jack_activate(jack_client);
    if (activate_result != 0) {
        throw OstrostrojException(std::format("Jack activate failed! [status=0x{:x}]", activate_result));        
    }
    spdlog::info("Jack client activated.");
    const auto connect_result = jack_connect(jack_client, MIDI_INPUT_PORT.c_str(), jack_port_name(midi_input_port));
    if (connect_result != 0) {
        throw OstrostrojException(std::format("Jack connect failed! [status=0x{:x}]", connect_result));        
    }
};

SoundCard::~SoundCard() {
    if (jack_client != nullptr) {
        jack_client_close(jack_client);
        spdlog::info("Jack client closed.");        
    }
};

/**
 * TODO:
 * - jack_set_port_connect_callback()
 * - jack_set_port_registration_callback()
*/