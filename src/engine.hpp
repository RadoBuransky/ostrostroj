#pragma once

#include "alsamidi.hpp"
#include "alsapcm.hpp"
#include "project.hpp"
#include "track.hpp"

static constexpr int ENGINE_LOOP_TRACKS = 6;
static constexpr int ENGINE_LOOP_MONO_TRACKS = 4;
static_assert(ENGINE_LOOP_MONO_TRACKS + (ENGINE_LOOP_TRACKS - ENGINE_LOOP_MONO_TRACKS) * 2 + 2 < PCM_OUT_CHANNELS);

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
        bool handle_midi_event(snd_seq_event_t& midi_event, PcmEvent& result);
        Program& set_program(int program_number);
    public:
        Engine(Project& _project, AlsaMidi& _alsa_midi, AlsaPcm& _alsa_pcm);
        virtual ~Engine();

        bool pcm_event_callback(PcmEvent& event, bool sync);
        void pcm_callback(PcmFrame_s24_3le& frame);
        void midi_callback();

        int get_loop_track_count() const;
};