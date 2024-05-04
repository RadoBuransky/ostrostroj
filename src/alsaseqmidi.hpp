#include <alsa/asoundlib.h>

class AlsaSeqMidi {
    private:
        snd_seq_t* client;
        int in_port;
        int out_port;

        snd_seq_t* open_client();
        int create_input_port();
        int create_output_port();
        void midi_action();

    public:
        AlsaSeqMidi();
        virtual ~AlsaSeqMidi();
};