#pragma once

typedef farbot::fifo<snd_seq_event_t,
            farbot::fifo_options::concurrency::single,
            farbot::fifo_options::concurrency::single,
            farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
            farbot::fifo_options::full_empty_failure_mode::overwrite_or_return_default> AlsaMidiFifo;

class AlsaMidi {
    private:
        std::vector<snd_rawmidi_t*> handle_ins;
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
        size_t read(snd_rawmidi_t* handle_in, unsigned char* raw, size_t size);
        bool poll_in(std::vector<pollfd>& poll_descriptors);
        void run();
        friend void* run_midi(void* context);
        pollfd create_poll_descriptors(snd_rawmidi_t *handle);
        std::vector<snd_rawmidi_t*> open_midi_ins();
        std::string find_midi_out_device_name();
        snd_rawmidi_t* open_midi_out(const std::string& device_name);
    public:
        AlsaMidi();
        virtual ~AlsaMidi();
        AlsaMidiFifo& get_fifo_in();
        void start(std::function<void(void)> _callback);
        void write(snd_seq_event_t event);
        void shutdown();
};