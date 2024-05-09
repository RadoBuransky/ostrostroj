#pragma once

#include "alsamidi.hpp"
#include "alsapcm.hpp"
#include "project.hpp"
#include "track.hpp"

static constexpr int ENGINE_LOOP_TRACKS = 6;

struct TrackState {
    InterleavedFifo* fifo;
    int channels;
};

struct EngineState {    
    PcmFifo& pcm_fifo;
    PcmFrame_s24_3le frame;
    bool pending;
    std::array<TrackState, ENGINE_LOOP_TRACKS + 1> tracks;
    EngineState(PcmFifo& _pcm_fifo);
    bool push_pending();
};

class Engine {
    private:
        Project& project;
        AlsaMidi& alsa_midi;
        AlsaPcm& alsa_pcm;
        std::array<std::unique_ptr<Track>, ENGINE_LOOP_TRACKS> loop_tracks; // 0-3 mono, 4-5 stereo
        Track one_shots_track;
        EngineState state;
        std::reference_wrapper<Program> program;
        std::atomic_bool stop;
        std::atomic_flag heartbeat;
        std::thread engine_thread;
        void run();
        void process_midi();
        void process_pcm();
        Program& set_program(int program_number);
    public:
        Engine(Project& _project, AlsaMidi& _alsa_midi, AlsaPcm& _alsa_pcm);
        virtual ~Engine();
        int get_loop_track_count() const;
        void pcm_callback();
        void midi_callback();
};