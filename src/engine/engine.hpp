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
#include "model_cycles.hpp"
#include "loop_encoders.hpp"
#include "command_controller.hpp"

class Engine {
    private:
        Workspace& workspace;
        AlsaMidi& alsa_midi;
        AlsaPcm& alsa_pcm;
        Display& display;
        std::unique_ptr<ModelCycles> model_cycles;
        LoopEncoders loop_encoders;
        CommandController command_controller;
        std::atomic_flag midi_flag;
        std::atomic_bool stop;
        std::array<std::unique_ptr<Track>, ENGINE_LOOP_TRACKS> loop_tracks; // 0-3 mono, 4-5 stereo
        Track one_shots_track;
        std::array<InterleavedFifo*, PCM_OUT_CHANNELS> track_fifos;
        useconds_t worker_sleep_time;
        std::vector<std::unique_ptr<EngineWorker>> workers;
        std::unique_ptr<Session> session;
        std::unique_ptr<PatternLearn> pattern_learn;
        bool handle_midi_event(snd_seq_event_t& midi_event, snd_pcm_state_t state, PcmEvent& result);
        void note(uint8_t channel, uint8_t note, bool on, unsigned int clock, bool running);
        void one_shot_note(uint8_t note, bool on);
        void controller(uint8_t channel, unsigned int param, signed int value, unsigned int clock, bool running);
        void update_saturation(ssize_t track_number);
        void learn(unsigned int param, signed int value, unsigned int clock);
        snd_pcm_uframes_t compute_latency(uint8_t mul, uint8_t clock_interval);
        void program_changed(bool running, snd_pcm_uframes_t predelay);
        void add_loop_clips(bool running, snd_pcm_uframes_t predelay);
        void clear_loop_clips(bool running);
        void lock_worker_tracks();
        void unlock_worker_tracks();
        void un_mute_mc_tracks();
        EngineWorker& get_worker(uint8_t track_number);
        void add_worker_track(std::map<size_t, std::vector<std::reference_wrapper<Track>>>& worker_tracks, Track& track);
        std::vector<std::unique_ptr<EngineWorker>> create_workers();
        std::array<InterleavedFifo*, PCM_OUT_CHANNELS> init_track_fifos();
    public:
        Engine(Workspace& _workspace, AlsaMidi& _alsa_midi, AlsaPcm& _alsa_pcm, Display& _display);
        virtual ~Engine();

        void shutdown();

        // pcm_* callbacks are called from ALSA PCM thread (never called concerruntly!)
        bool pcm_event_callback(PcmEvent& event, snd_pcm_state_t state, bool sync);
        void pcm_callback(PcmFrame_s24_3le& frame);

        // Called from ALSA MIDI thread
        void midi_callback();

        int get_loop_track_count() const;
};