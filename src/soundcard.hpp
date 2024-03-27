#pragma once

#include <string>
#include <jack/jack.h>
#include <libremidi/configurations.hpp>
#include <libremidi/libremidi.hpp>
#include "farbot/fifo.hpp"
#include "engine.hpp"

class AudioPortFifo {
    private:
        jack_port_t* const port;
        std::unique_ptr<AudioFifo> fifo;
    public:
        AudioPortFifo(jack_port_t* const port, jack_nframes_t buffer_size);

        jack_port_t* get_port() const;
        void copy_to_buffer(const jack_nframes_t nframes) const;
};

class SoundCard {
    private:
        inline static const std::string INPUT_MIDI_PORT = "system_midi:capture_1";
        inline static const std::string AUDIO_OUTPUT_PORT_PREFIX = "alsa_pcm:hw:UMC1820:in";
        static constexpr int AUDIO_OUTPUT_PORT_COUNT = 10;
        inline static const std::string LOCAL_MIDI_PORT = "libremidi_input";
        inline static const std::string LOCAL_AUDIO_OUTPUT_PORT_PREFIX = "audio_output_";
        jack_client_t * const jack_client;
        std::vector<libremidi::jack_callback> midiin_callbacks;
        libremidi::midi_in midiin;
        std::unique_ptr<MidiFifo> midi_fifo;
        const std::vector<AudioPortFifo> audio_outputs;
        const jack_nframes_t buffer_size;
        std::function<void(void)> callback = {};
        
        static int process_callback(jack_nframes_t nframes, void *arg);
        void libremidi_message_callback(const libremidi::message& message);

        std::vector<AudioPortFifo> create_audio_outputs(jack_client_t * jack_client);
        static void port_connect_callback(jack_port_id_t a, jack_port_id_t b, int connect, void*);
        static void port_registration_callback(jack_port_id_t port, int registered, void*);
        static jack_client_t * create_client(const std::string &name);
        libremidi::midi_in create_midiin(std::vector<libremidi::jack_callback>  & midiin_callbacks, jack_client_t * jack_client);

        void registerCallbacks();
        void activate();
        void connect(jack_client_t * jack_client);

    public:
        SoundCard(const std::string &);
        virtual ~SoundCard();

        void start(std::function<void(void)>);

        int get_sample_rate() const;
        int get_audio_outputs() const;
        jack_nframes_t get_buffer_size() const;
};