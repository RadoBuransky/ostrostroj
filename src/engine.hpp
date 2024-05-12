#pragma once

#include "alsamidi.hpp"
#include "alsapcm.hpp"
#include "project.hpp"
#include "track.hpp"

static constexpr int ENGINE_LOOP_TRACKS = 6;
static constexpr int ENGINE_LOOP_MONO_TRACKS = 4;
static_assert(ENGINE_LOOP_MONO_TRACKS + (ENGINE_LOOP_TRACKS - ENGINE_LOOP_MONO_TRACKS) * 2 + 2 < PCM_OUT_CHANNELS);

class EngineWorker {
    private:
        int worker_index;
        useconds_t sleep_time;
        std::atomic_bool stop;
        std::mutex mutex;
        std::condition_variable cv;
        std::vector<std::reference_wrapper<Track>> tracks;
        std::thread thread;
        void run();
        bool run_tracks();
    public:
        EngineWorker(int _worker_index, useconds_t _sleep_time);
        virtual ~EngineWorker();
        void assign_tracks(std::vector<std::reference_wrapper<Track>> _tracks);
        void release_tracks();
};

class Engine {
    private:
        Project& project;
        AlsaMidi& alsa_midi;
        AlsaPcm& alsa_pcm;
        std::atomic_flag midi_flag;
        std::array<std::unique_ptr<Track>, ENGINE_LOOP_TRACKS> loop_tracks; // 0-3 mono, 4-5 stereo
        Track one_shots_track;
        std::reference_wrapper<Program> program;
        std::array<InterleavedFifo*, PCM_OUT_CHANNELS> track_fifos;
        useconds_t worker_sleep_time;
        std::vector<std::unique_ptr<EngineWorker>> workers;
        bool handle_midi_event(snd_seq_event_t& midi_event, bool running, PcmEvent& result);
        void drop_track_fifos();
        void change_program(int program_number, bool running);
        void update_tracks();
        void reset_program(bool running);
        void assign_worker_tracks();
        void release_worker_tracks();
        std::vector<std::unique_ptr<EngineWorker>> create_workers();
    public:
        Engine(Project& _project, AlsaMidi& _alsa_midi, AlsaPcm& _alsa_pcm);
        virtual ~Engine();

        // pcm_* callbacks are called from ALSA PCM thread (never called concerruntly!)
        bool pcm_event_callback(PcmEvent& event, bool running, bool sync);
        void pcm_callback(PcmFrame_s24_3le& frame);

        // Called from ALSA MIDI thread
        void midi_callback();

        int get_loop_track_count() const;
};