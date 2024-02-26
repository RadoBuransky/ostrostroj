#pragma once

#include "jack/jack.h"
#include <string>

class SoundCard {
    private:
        inline static const std::string MIDI_INPUT_PORT = "system_midi:capture_1";
        jack_client_t * const jack_client;
        jack_port_t * const midi_input_port;

        static int process_callback(jack_nframes_t nframes, void *arg);
        static void port_connect_callback(jack_port_id_t a, jack_port_id_t b, int connect, void*);
        static void port_registration_callback(jack_port_id_t port, int registered, void*);
        static jack_client_t * create_client(const std::string &name, SoundCard *);
        static jack_port_t * create_midi_input_port(jack_client_t * jack_client);

        void registerCallbacks();

    public:
        SoundCard(const std::string &);
        virtual ~SoundCard();
};