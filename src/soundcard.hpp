#pragma once

#include <string>
#include <jack/jack.h>
#include <libremidi/configurations.hpp>
#include <libremidi/libremidi.hpp>

class SoundCard {
    private:
        inline static const std::string INPUT_MIDI_PORT = "system_midi:capture_1";
        inline static const std::string LOCAL_MIDI_PORT = "libremidi_input";
        jack_client_t * const jack_client;
        std::vector<libremidi::jack_callback> midiin_callbacks;
        libremidi::midi_in midiin;

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