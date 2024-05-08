#pragma once

#include "alsamidi.hpp"
#include "alsapcm.hpp"
#include "project.hpp"
#include "track.hpp"

class Engine {
    private:
        Project& project;
        AlsaMidi& alsa_midi;
        AlsaPcm& alsa_pcm;
        std::array<std::unique_ptr<Track>, 6> loop_tracks; // 0-3 mono, 4-5 stereo
        Track one_shots_track;
        std::reference_wrapper<Program> program;
        std::atomic_bool stop;
        std::atomic_flag heartbeat;
        std::thread engine_thread;
        void run();
        void process_midi();
        void process_pcm(PcmFrame_s24_3le& frame);
        Program& set_program(int program_number);
    public:
        Engine(Project& _project, AlsaMidi& _alsa_midi, AlsaPcm& _alsa_pcm);
        virtual ~Engine();
        int get_loop_track_count() const;
        void pcm_callback();
        void midi_callback();
};