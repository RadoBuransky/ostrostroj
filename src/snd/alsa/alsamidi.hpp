#pragma once
#include <alsa/asoundlib.h>
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
        snd_midi_event_t* parser_in;
        snd_midi_event_t* parser_out;
        std::atomic_bool stop;
        AlsaMidiFifo fifo_in;
        snd_seq_tick_time_t clock_counter;
        std::chrono::steady_clock::time_point last_clock;
        std::chrono::steady_clock::duration clock_interval;
        pthread_t thru_thread;
        std::function<void(void)> callback;
        bool process(snd_seq_event_t& event);
        void write(unsigned char* raw, size_t size);
        void parse(unsigned char* raw, size_t read_size);
        size_t read(unsigned char* raw, size_t size);
        bool poll_in(std::vector<pollfd>& poll_descriptors);
        void run();
        friend void* run_midi(void* context);
        std::vector<pollfd> create_poll_descriptors(snd_rawmidi_t *handle);
        snd_rawmidi_t* open_midi_in(const std::string& device_name);
        snd_rawmidi_t* open_midi_out(const std::string& device_name);
    public:
        AlsaMidi();
        virtual ~AlsaMidi();
        AlsaMidiFifo& get_fifo_in();
        void start(std::function<void(void)> _callback);
        void write(snd_seq_event_t event);
        void shutdown();
};