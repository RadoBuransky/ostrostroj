#define SPDLOG_ACTIVE_LEVEL 2

#include "common.hpp"
#include "engine.hpp"

static constexpr std::chrono::microseconds TRACK_XRUN_SLEEP = std::chrono::microseconds(100);

bool Engine::handle_midi_event(snd_seq_event_t& midi_event, PcmEvent& result) {
    switch(midi_event.type) {
        case SND_SEQ_EVENT_START: 
            SPDLOG_INFO("Engine MIDI START [d0={},d1={},queue={}]", midi_event.data.queue.param.d32[0], midi_event.data.queue.param.d32[1],
                midi_event.data.queue.queue);
            result = ALSA_PCM_START;
            return true;
        case SND_SEQ_EVENT_STOP: 
            SPDLOG_INFO("Engine MIDI STOP [d0={},d1={},queue={}]", midi_event.data.queue.param.d32[0], midi_event.data.queue.param.d32[1],
                midi_event.data.queue.queue);
            result = ALSA_PCM_PAUSE;
            return true;
        case SND_SEQ_EVENT_CONTINUE: 
            SPDLOG_INFO("Engine MIDI CONTINUE [d0={},d1={},queue={}]", midi_event.data.queue.param.d32[0], midi_event.data.queue.param.d32[1],
                midi_event.data.queue.queue);
            result = ALSA_PCM_RESUME;
            return true;
        case SND_SEQ_EVENT_PGMCHANGE: 
            SPDLOG_INFO("Engine MIDI PROGRAM CHANGE [param={},value={}]", midi_event.data.control.param, midi_event.data.control.value);
            // TODO: xfade tracks
            set_program(midi_event.data.control.value + 1);
            result = ALSA_PCM_PROGRAM_CHANGE;
            return true;
        default:
            SPDLOG_WARN("Ignored engine MIDI event. [{}]", (int)midi_event.type);
            return false;
    }   
}

Program& Engine::set_program(int program_number) {
    program = project.get_program(program_number);
    // TODO: ...
    // for (size_t track = 0; track < loop_tracks.size(); track++) {
    //     loop_tracks.at(track)->reset_node();
    //     state.tracks.at(track).fifo = nullptr;
    //     state.tracks.at(track).channels = loop_tracks.at(track)->get_channels();
    // }
    // for (LoopClip& loop_clip : program.get().get_loops()) {
    //     int track = loop_clip.get_track();
    //     loop_tracks.at(track)->set_node(std::make_unique<ClipNode>(loop_clip, true));
    //     state.tracks.at(track).fifo = &loop_tracks.at(track)->get_fifo();
    //     state.tracks.at(track).channels = loop_tracks.at(track)->get_channels();
    // }
    // state.tracks.at(loop_tracks.size()).fifo = &one_shots_track.get_fifo();
    // state.tracks.at(loop_tracks.size()).channels = one_shots_track.get_channels();
    SPDLOG_INFO("Program set = {}", program.get().get_start_number());
    return program;
}

Engine::Engine(Project& _project, AlsaMidi& _alsa_midi, AlsaPcm& _alsa_pcm):
    project(_project),
    alsa_midi(_alsa_midi),
    alsa_pcm(_alsa_pcm),
    midi_flag(ATOMIC_FLAG_INIT),
    loop_tracks {
        std::make_unique<Track>(1, 1, _alsa_pcm.get_period_size(), false),
        std::make_unique<Track>(2, 1, _alsa_pcm.get_period_size(), false),
        std::make_unique<Track>(3, 1, _alsa_pcm.get_period_size(), false),
        std::make_unique<Track>(4, 1, _alsa_pcm.get_period_size(), false),
        std::make_unique<Track>(5, 2, _alsa_pcm.get_period_size(), false),
        std::make_unique<Track>(6, 2, _alsa_pcm.get_period_size(), false),
    },
    one_shots_track(Track(7, 2, _alsa_pcm.get_period_size(), true)),
    program(set_program(1)) {
    // TODO: Create worker threads and call Track::run
}

Engine::~Engine() {
}

bool Engine::pcm_event_callback(PcmEvent& event, bool sync) {
    snd_seq_event_t midi_event;
    if (alsa_midi.get_fifo().pop(midi_event)) {
        return handle_midi_event(midi_event, event);
    } else {
        if (sync) {
            midi_flag.test_and_set();
            midi_flag.wait(true);
            if (!alsa_midi.get_fifo().pop(midi_event)) {
                throw OstrostrojException("Engine MIDI FIFO empty!");
            }
            return handle_midi_event(midi_event, event);
        }
        return false;
    }
}

bool Engine::pcm_callback(PcmFrame_s24_3le& frame) {
//     TrackState* track;
//     int ch;
//     PcmSample_s24_3le* sample;
//     SPDLOG_DEBUG("Engine processing PCM...");
//     while (state.push_pending()) {
//         sample = state.frame.channels.data();
//         track = state.tracks.data();
//         while (track != state.tracks.end()) {
//             ch = track->channels;
//             while (ch-- > 0) {
//                 if (track->fifo != nullptr) {
//                     if (!track->fifo->pop(*sample)) {
//                         int track_number = (track - state.tracks.data()) + 1;
//                         SPDLOG_WARN("Engine track {} xrun...", track_number);                        
//                         do {
//                             usleep(TRACK_XRUN_SLEEP.count());
//                         } while (!track->fifo->pop(*sample));
//                         SPDLOG_DEBUG("Engine track {} underrun recovered.", track_number);
//                     }
//                 } else {
//                     sample->silence();
//                 }
//                 sample++;
// #ifndef NDEBUG
//                 if (sample >= state.frame.channels.end()) {
//                     throw OstrostrojException("sample pointer overflow!");
//                 }
// #endif
//             }
//             track++;
//         }
//         state.pending = true;
//     }
//     SPDLOG_DEBUG("Engine processing PCM done");
    return false;
}

void Engine::midi_callback() {
    midi_flag.clear();
    midi_flag.notify_one();
}

int Engine::get_loop_track_count() const {
    return loop_tracks.size();    
}