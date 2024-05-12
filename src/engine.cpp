#define SPDLOG_ACTIVE_LEVEL 2

#include "common.hpp"
#include "engine.hpp"

static constexpr std::chrono::microseconds TRACK_XRUN_SLEEP = std::chrono::microseconds(100);
static constexpr int TRACK_XRUN_RETRY = 100;

EngineState::EngineState(PcmFifo& _pcm_fifo):
    fifo(_pcm_fifo),
    frame(),
    pending(false) {    
}

bool EngineState::push_pending() {
    if (pending) {
        if (fifo.push(std::move(frame))) {
            pending = false;
            return true;
        }
        return false;
    }
    return true;
}

void Engine::run() {
    try {
        SPDLOG_INFO("Engine started.");
        while (!stop) {
            process_midi();
            process_pcm();
            if (heartbeat.test_and_set()) {
                heartbeat.wait(true);
                heartbeat.test_and_set();
            } else {
                SPDLOG_DEBUG("Got another heartbeat in the meantime.");
            }
        }
        SPDLOG_INFO("Engine stopped.");
    } catch(std::exception const& e) {
        SPDLOG_ERROR("Engine failed. {}", e.what());
    }
}

void Engine::process_midi() {
    snd_seq_event_t midi_event;
    while (alsa_midi.get_fifo().pop(midi_event)) {
        switch(midi_event.type) {
            case SND_SEQ_EVENT_START: 
                SPDLOG_INFO("Engine MIDI START [d0={},d1={},queue={}]", midi_event.data.queue.param.d32[0], midi_event.data.queue.param.d32[1],
                    midi_event.data.queue.queue);
                alsa_pcm.play_start();
                break;
            case SND_SEQ_EVENT_STOP: 
                SPDLOG_INFO("Engine MIDI STOP [d0={},d1={},queue={}]", midi_event.data.queue.param.d32[0], midi_event.data.queue.param.d32[1],
                    midi_event.data.queue.queue);
                alsa_pcm.play_stop();
                break;
            case SND_SEQ_EVENT_CONTINUE: 
                SPDLOG_INFO("Engine MIDI CONTINUE [d0={},d1={},queue={}]", midi_event.data.queue.param.d32[0], midi_event.data.queue.param.d32[1],
                    midi_event.data.queue.queue);
                alsa_pcm.play_continue();
                break;
            case SND_SEQ_EVENT_PGMCHANGE: 
                SPDLOG_INFO("Engine MIDI PROGRAM CHANGE [param={},value={}]", midi_event.data.control.param, midi_event.data.control.value);
                // TODO: xfade tracks
                set_program(midi_event.data.control.value + 1);
                alsa_pcm.play_program_change();
                break;
            default:
                SPDLOG_WARN("Ignored engine MIDI event. [{}]", (int)midi_event.type);
                break;
        }
    }
}

void Engine::process_pcm() {
    TrackState* track;
    int ch;
    PcmSample_s24_3le* sample;
    SPDLOG_DEBUG("Engine processing PCM...");
    while (state.push_pending()) {
        sample = state.frame.channels.data();
        track = state.tracks.data();
        while (track != state.tracks.end()) {
            ch = track->channels;
            while (ch-- > 0) {
                if (track->fifo != nullptr) {
                    if (!track->fifo->pop(*sample)) {
                        int track_number = (track - state.tracks.data()) + 1;
                        SPDLOG_WARN("Engine track {} xrun...", track_number);                        
                        int retry = TRACK_XRUN_RETRY;
                        do {
                            usleep(TRACK_XRUN_SLEEP.count());
                        } while (retry-- > 0 && !track->fifo->pop(*sample));
                        if (retry == 0) {
                            // TODO: Remove this, just keep retrying. Track would desync!
                            SPDLOG_WARN("Engine track {} underrun!", track_number);
                            sample->silence();
                        } else {
                            SPDLOG_DEBUG("Engine track {} underrun recovered.", track_number);
                        }
                    }
                } else {
                    sample->silence();
                }
                sample++;
#ifndef NDEBUG
                if (sample >= state.frame.channels.end()) {
                    throw OstrostrojException("sample pointer overflow!");
                }
#endif
            }
            track++;
        }
        state.pending = true;
    }
    SPDLOG_DEBUG("Engine processing PCM done");
}

Program& Engine::set_program(int program_number) {
    state.frame.silence();
    state.pending = false;
    program = project.get_program(program_number);    
    for (size_t track = 0; track < loop_tracks.size(); track++) {
        loop_tracks.at(track)->reset_node();
        state.tracks.at(track).fifo = nullptr;
        state.tracks.at(track).channels = loop_tracks.at(track)->get_channels();
    }
    for (LoopClip& loop_clip : program.get().get_loops()) {
        int track = loop_clip.get_track();
        loop_tracks.at(track)->set_node(std::make_unique<ClipNode>(loop_clip, true));
        state.tracks.at(track).fifo = &loop_tracks.at(track)->get_fifo();
        state.tracks.at(track).channels = loop_tracks.at(track)->get_channels();
    }
    state.tracks.at(loop_tracks.size()).fifo = &one_shots_track.get_fifo();
    state.tracks.at(loop_tracks.size()).channels = one_shots_track.get_channels();
    SPDLOG_INFO("Program set = {}", program.get().get_start_number());
    return program;
}

Engine::Engine(Project& _project, AlsaMidi& _alsa_midi, AlsaPcm& _alsa_pcm):
    project(_project),
    alsa_midi(_alsa_midi),
    alsa_pcm(_alsa_pcm),
    loop_tracks {
        std::make_unique<Track>(1, 1, _alsa_pcm.get_period_time(), _alsa_pcm.get_period_size(), false),
        std::make_unique<Track>(2, 1, _alsa_pcm.get_period_time(), _alsa_pcm.get_period_size(), false),
        std::make_unique<Track>(3, 1, _alsa_pcm.get_period_time(), _alsa_pcm.get_period_size(), false),
        std::make_unique<Track>(4, 1, _alsa_pcm.get_period_time(), _alsa_pcm.get_period_size(), false),
        std::make_unique<Track>(5, 2, _alsa_pcm.get_period_time(), _alsa_pcm.get_period_size(), false),
        std::make_unique<Track>(6, 2, _alsa_pcm.get_period_time(), _alsa_pcm.get_period_size(), false),
    },
    one_shots_track(Track(7, 2, _alsa_pcm.get_period_time(), _alsa_pcm.get_period_size(), true)),
    state(_alsa_pcm.get_pcm_fifo()),
    program(set_program(1)),
    stop(false),
    engine_thread(std::bind(&Engine::run, this)) {
    pthread_setname_np(engine_thread.native_handle(), "engine");
}

Engine::~Engine() {
    stop = true;
    engine_thread.join();
}

int Engine::get_loop_track_count() const {
    return loop_tracks.size();    
}

void Engine::pcm_callback() {
    heartbeat.clear();
    heartbeat.notify_one();
}

void Engine::midi_callback() {
    heartbeat.clear();
    heartbeat.notify_one();
}