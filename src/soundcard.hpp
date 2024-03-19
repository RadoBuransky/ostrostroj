#pragma once

#include <string>
#include <jack/jack.h>
#include <libremidi/configurations.hpp>
#include <libremidi/libremidi.hpp>
#include "farbot/fifo.hpp"

class PortFifo {
    public:
        PortFifo(jack_port_t* port, jack_nframes_t buffer_size);
        PortFifo(const PortFifo& other);
        PortFifo(PortFifo&& other);
        jack_port_t* port;
        const farbot::fifo<jack_default_audio_sample_t,
            farbot::fifo_options::concurrency::single,
            farbot::fifo_options::concurrency::single,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
            1>& queue;
};

class SoundCard {
    private:
        inline static const std::string INPUT_MIDI_PORT = "system_midi:capture_1";
        inline static const std::string AUDIO_OUTPUT_PORT_PREFIX = "alsa_pcm:hw:UMC1820:in";
        inline static const std::string LOCAL_MIDI_PORT = "libremidi_input";
        inline static const std::string LOCAL_AUDIO_OUTPUT_PORT_PREFIX = "audio_output_";
        jack_client_t * const jack_client;
        std::vector<libremidi::jack_callback> midiin_callbacks;
        libremidi::midi_in midiin;
        const std::vector<PortFifo> audio_outputs;

        std::vector<PortFifo> create_audio_outputs(jack_client_t * jack_client);
        static int process_callback(jack_nframes_t nframes, void *arg);
        static void port_connect_callback(jack_port_id_t a, jack_port_id_t b, int connect, void*);
        static void port_registration_callback(jack_port_id_t port, int registered, void*);
        static jack_client_t * create_client(const std::string &name);
        static libremidi::midi_in create_midiin(std::vector<libremidi::jack_callback>  & midiin_callbacks, jack_client_t * jack_client);

        static void libremidi_message_callback(const libremidi::message& message);

        void registerCallbacks();
        void activate();
        void connect(jack_client_t * jack_client);

    public:
        SoundCard(const std::string &);
        virtual ~SoundCard();

        int get_sample_rate() const;
};