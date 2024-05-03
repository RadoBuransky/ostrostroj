#include <alsa/rawmidi.h>

static void* run_thru(void* context);

class AlsaMidi {
    private:
        // $ amidi -l
        static constexpr std::string device = "hw:0,0,0";
        snd_rawmidi_t *handle_in;
        snd_rawmidi_t *handle_out;
        std::atomic_bool stop;
        pthread_t thru_thread;
        friend void* run_thru(void* context);
        snd_rawmidi_t* open_in(const std::string& device_name);
        snd_rawmidi_t* open_out(const std::string& device_name);
        pthread_t create_rt_thread();
    public:
        AlsaMidi();
        virtual ~AlsaMidi();
};