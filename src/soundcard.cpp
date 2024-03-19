#include <string>
#include <format>
#include <spdlog/spdlog.h>
#include <jack/midiport.h>
#include <libremidi/backends/jack/midi_in.hpp>
#include "soundcard.hpp"
#include "common.hpp"

PortFifo::PortFifo(jack_port_t* port, jack_nframes_t buffer_size) :
    port(port),
    queue(farbot::fifo<jack_default_audio_sample_t,
            farbot::fifo_options::concurrency::single,
            farbot::fifo_options::concurrency::single,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
            1>(buffer_size)) {
}

PortFifo::PortFifo(const PortFifo& other):
    port(other.port),
    queue(std::move(other.queue)) {
}

PortFifo::PortFifo(PortFifo&& other):
    port(other.port),
    queue(std::move(other.queue)) {
}

SoundCard::SoundCard(const std::string &name) :
    jack_client(create_client(name)),
    midiin_callbacks({}),
    midiin(create_midiin(midiin_callbacks, jack_client)),
    audio_outputs(create_audio_outputs(jack_client)) {
    registerCallbacks();
    const auto buffer_size = jack_get_buffer_size(jack_client);
    activate();
    connect(jack_client);
    jack_latency_range_t latency_range;
    jack_port_get_latency_range(audio_outputs.at(0).port, JackLatencyCallbackMode::JackPlaybackLatency, &latency_range);
    spdlog::info(std::format("Jack client activated. [{} Hz, {} frames, latency {} - {}]", jack_get_sample_rate(jack_client),
        buffer_size, latency_range.min, latency_range.max));
}

SoundCard::~SoundCard() {
    midiin.close_port();
    for (const PortFifo& audio_output : audio_outputs) {
        jack_port_unregister(jack_client, audio_output.port);
    }
    jack_client_close(jack_client);
    spdlog::info("Jack client closed.");
}

int SoundCard::process_callback(jack_nframes_t nframes, void *arg) {    
    try {
        auto& self = *(SoundCard*)arg;    
        // Process the midi inputs
        for (auto &midiin_callback: self.midiin_callbacks) {
            midiin_callback.callback(nframes);
        }
        // TODO: jack_default_audio_sample_t* out = jack_port_get_buffer();
    } catch (std::exception const& ex) {
        spdlog::error(ex.what());
    }
    return 0;
}

std::vector<PortFifo> SoundCard::create_audio_outputs(jack_client_t * jack_client) {
    const auto buffer_size = jack_get_buffer_size(jack_client);
    std::vector<PortFifo> result = {};
    for (auto i = 1; i <= 10; i++) {
        const auto jack_port = jack_port_register(jack_client, std::format("{}{}", LOCAL_AUDIO_OUTPUT_PORT_PREFIX, i).c_str(),
            JACK_DEFAULT_AUDIO_TYPE, JackPortFlags::JackPortIsOutput, 0);
        auto port_fifo = PortFifo(jack_port, buffer_size);
        result.push_back(port_fifo);
    }
    return result;
}

libremidi::midi_in SoundCard::create_midiin(std::vector<libremidi::jack_callback> & midiin_callbacks, jack_client_t * jack_client) {
    auto api_input_config = libremidi::jack_input_configuration{
        .context = jack_client,
        .set_process_func = [&midiin_callbacks](libremidi::jack_callback cb) {
            midiin_callbacks.push_back(std::move(cb));
        },
        .clear_process_func = [&midiin_callbacks](int) {
            midiin_callbacks.clear();
        }
    };
    libremidi::midi_in result = libremidi::midi_in(
          libremidi::input_configuration{
            .on_message = [=](libremidi::message m) {
                libremidi_message_callback(m);
            },
            .get_timestamp = [=](int64_t t) {
                return t;
            }},
            api_input_config);
    result.open_virtual_port(LOCAL_MIDI_PORT);
    return result;
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

jack_client_t * SoundCard::create_client(const std::string &name) {
    jack_status_t status;
    auto jack_client = jack_client_open(name.c_str(), JackNoStartServer, &status);
    if (nullptr == jack_client) {        
        throw OstrostrojException(std::format("Jack client open failed! [status=0x{:x}]", static_cast<int>(status)));
    }
    spdlog::info(std::format("Jack client open. [status=0x{:x}]", static_cast<int>(status)));
    return jack_client;
}

void SoundCard::libremidi_message_callback(const libremidi::message& message) {
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
    spdlog::info(std::format("{} ch{}", log_message, message.get_channel()));
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
}

void SoundCard::connect(jack_client_t * jack_client) {
    std::string destination_port = std::format("{}:{}", jack_get_client_name(jack_client), LOCAL_MIDI_PORT);
    const auto connect_result = jack_connect(jack_client, INPUT_MIDI_PORT.c_str(), destination_port.c_str());
    if (connect_result != 0) {
        throw OstrostrojException(std::format("Jack connect failed! [status=0x{:x}]", connect_result));        
    }
    spdlog::debug("MIDI ports connected.");

    for (auto i = 0; i < audio_outputs.size(); i++) {
        const auto src_port = jack_port_name(audio_outputs.at(i).port);
        const auto dst_port = std::format("{}{}", AUDIO_OUTPUT_PORT_PREFIX, i + 1);
        const auto connect_result = jack_connect(jack_client, src_port, dst_port.c_str());
        if (connect_result != 0) {
            throw OstrostrojException(std::format("Jack connect failed! [status=0x{:x}]", connect_result));        
        }        
    }
    spdlog::debug("Audio output ports connected.");
}

int SoundCard::get_sample_rate() const {
    return jack_get_sample_rate(jack_client);
}