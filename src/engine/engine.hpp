#pragma once

#include "alsamidi.hpp"
#include "alsapcm.hpp"
#include "workspace.hpp"
#include "project.hpp"
#include "track.hpp"
#include "engineworker.hpp"
#include "session.hpp"
#include "display.hpp"
#include "pattern_learn.hpp"

static constexpr int ENGINE_LOOP_TRACKS = 6;
static constexpr int ENGINE_LOOP_MONO_TRACKS = 4;
static_assert(ENGINE_LOOP_MONO_TRACKS + (ENGINE_LOOP_TRACKS - ENGINE_LOOP_MONO_TRACKS) * 2 + 2 < PCM_OUT_CHANNELS);

class Engine {
    private:
        Workspace& workspace;
        AlsaMidi& alsa_midi;
        AlsaPcm& alsa_pcm;
        Display& display;
        std::atomic_flag midi_flag;
        std::atomic_bool stop;
        std::array<std::unique_ptr<Track>, ENGINE_LOOP_TRACKS> loop_tracks; // 0-3 mono, 4-5 stereo
        Track one_shots_track;
        std::array<InterleavedFifo*, PCM_OUT_CHANNELS> track_fifos;
        useconds_t worker_sleep_time;
        std::vector<std::unique_ptr<EngineWorker>> workers;
        std::unique_ptr<Session> session;
        std::unique_ptr<PatternLearn> pattern_learn;
        bool handle_midi_event(snd_seq_event_t& midi_event, bool running, PcmEvent& result);
        void drop_track_fifos();
        void program_changed(bool running);
        void add_loop_clips();
        void clear_loop_clips(bool running);
        void lock_worker_tracks();
        void release_worker_tracks();
        std::vector<std::unique_ptr<EngineWorker>> create_workers();
    public:
        Engine(Workspace& _workspace, AlsaMidi& _alsa_midi, AlsaPcm& _alsa_pcm, Display& _display);
        virtual ~Engine();

        void shutdown();

        // pcm_* callbacks are called from ALSA PCM thread (never called concerruntly!)
        bool pcm_event_callback(PcmEvent& event, bool running, bool sync);
        void pcm_callback(PcmFrame_s24_3le& frame);

        // Called from ALSA MIDI thread
        void midi_callback();

        int get_loop_track_count() const;
};