#pragma once

#include "alsamidi.hpp"
#include "alsapcm.hpp"
#include "project.hpp"
#include "track.hpp"
#include "engineworker.hpp"
#include "session.hpp"
#include "display.hpp"
#include "loop_encoders.hpp"
#include "command_controller.hpp"
#include "program_change.hpp"

enum EngineExit {
    ENGINE_EXIT_NOOP = 0,
    ENGINE_EXIT_RESTART_DEVICE,
    ENGINE_EXIT_SHUTDOWN_DEVICE,
    ENGINE_EXIT_RESTART_SERVICE
};

class Engine {
    friend class ProgramChange;
    private:
        AlsaMidi alsa_midi;
        Session session;
        AlsaPcm alsa_pcm;
        Display& display;
        std::atomic_flag& running_flag;
        LoopEncoders loop_encoders;
        MidiEncoder fade_encoder;
        CommandController command_controller;
        std::unique_ptr<ProgramChange> program_change;
        std::atomic_flag midi_flag;
        std::atomic_bool stop;
        std::array<std::unique_ptr<Track>, ENGINE_LOOP_TRACKS> loop_tracks; // 0-4 mono, 5 stereo
        std::array<InterleavedFifo*, PCM_OUT_CHANNELS> track_fifos;
        useconds_t worker_sleep_time;
        std::vector<std::unique_ptr<EngineWorker>> workers;
        EngineExit exit_code;
        snd_pcm_uframes_t last_computed_latency;
        bool handle_midi_event(snd_seq_event_t& midi_event, snd_pcm_state_t state, PcmEvent& result);
        void note(uint8_t channel, uint8_t note, bool on, unsigned int clock, bool running);
        void exit(EngineExit _exit_code);
        void on_clock(uint8_t quarter_note_fraction);
        void on_controller(uint8_t channel, unsigned int param, signed int value, bool running);
        void update_saturation(ssize_t track_number);
        snd_pcm_uframes_t compute_latency(uint8_t mul, uint8_t clock_interval);
        bool change_program(bool running, BankPattern bank_pattern);
        std::vector<std::reference_wrapper<ClipPlayer>> add_loop_clips(Pattern& pattern);
        void remove_clip_player(Clip& clip);
        void clear_loop_clips(bool running);
        void lock_worker_tracks();
        void unlock_worker_tracks();
        EngineWorker& get_worker(uint8_t track_number);
        void add_worker_track(std::map<size_t, std::vector<std::reference_wrapper<Track>>>& worker_tracks, Track& track);
        std::vector<std::unique_ptr<EngineWorker>> create_workers();
        std::array<InterleavedFifo*, PCM_OUT_CHANNELS> init_track_fifos();
        void init_display();
        void midi_callback();
    public:
        Engine(Project& _project, Display& _display, std::atomic_flag& _running_flag);
        virtual ~Engine();

        void shutdown();

        // pcm_* callbacks are called from ALSA PCM thread (never called concerruntly!)
        bool pcm_event_callback(PcmEvent& event, snd_pcm_state_t state, bool sync);
        void pcm_callback(PcmFrame_s24_3le& frame);

        int get_loop_track_count() const;
        EngineExit get_exit_code() const;

};