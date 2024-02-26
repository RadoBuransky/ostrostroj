#pragma once

#include "jack/jack.h"
#include <string>

class SoundCard {
    private:
        inline static const std::string MIDI_INPUT_PORT = "system_midi:capture_1";
        jack_client_t * const jack_client;
        jack_port_t * const midi_input_port;

    public:
        SoundCard(const std::string &);
        virtual ~SoundCard();
};