#pragma once
#include "alsapcm.hpp"
#include <alsa/rawmidi.h>

class AlsaMidi {
    private:
        // $ amidi -l
        static constexpr std::string MIDI_DEVICE_NAME = "hw:0,0,0";
        AlsaPcm& alsa_pcm;
        snd_rawmidi_t *handle_in;
        snd_rawmidi_t *handle_out;
        std::atomic_bool stop;
        pthread_t thru_thread;
        friend void* run_thru(void* context);
        snd_rawmidi_t* open_midi_in(const std::string& device_name);
        snd_rawmidi_t* open_midi_out(const std::string& device_name);
    public:
        AlsaMidi(AlsaPcm& alsa_pcm);
        virtual ~AlsaMidi();
};