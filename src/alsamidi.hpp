#pragma once
#include <alsa/rawmidi.h>
#include "farbot/fifo.hpp"

typedef farbot::fifo<snd_seq_event_t,
            farbot::fifo_options::concurrency::single,
            farbot::fifo_options::concurrency::single,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
            farbot::fifo_options::full_empty_failure_mode::overwrite_or_return_default> AlsaMidiFifo;

class AlsaMidi {
    private:
        static constexpr std::string MIDI_DEVICE_NAME = "hw:0,0,0";
        snd_rawmidi_t *handle_in;
        snd_rawmidi_t *handle_out;
        std::atomic_bool stop;
        AlsaMidiFifo fifo;
        pthread_t thru_thread;
        friend void* run_thru(void* context);
        snd_rawmidi_t* open_midi_in(const std::string& device_name);
        snd_rawmidi_t* open_midi_out(const std::string& device_name);
    public:
        AlsaMidi();
        virtual ~AlsaMidi();
        AlsaMidiFifo& get_fifo();
};