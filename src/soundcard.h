#pragma once

#include "jack/jack.h"
#include <string>

class SoundCard {
    private:
        jack_client_t * const jack_client;

    public:
        SoundCard(const std::string &);
        virtual ~SoundCard();
};