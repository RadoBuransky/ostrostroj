#define SPDLOG_ACTIVE_LEVEL 1

#include "common.hpp"
#include "engine.hpp"

bool EngineState::push_pending() {
    if (pending) {
        if (pcm_fifo.push(std::move(frame))) {
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
                SPDLOG_DEBUG("Engine MIDI START");
                alsa_pcm.play_start();
                break;
            case SND_SEQ_EVENT_STOP: 
                SPDLOG_DEBUG("Engine MIDI STOP");
                alsa_pcm.play_stop();
                break;
            case SND_SEQ_EVENT_CONTINUE: 
                SPDLOG_DEBUG("Engine MIDI CONTINUE");
                alsa_pcm.play_continue();
                break;
            case SND_SEQ_EVENT_PGMCHANGE: 
                SPDLOG_DEBUG("Engine MIDI PROGRAM CHANGE [{}]", midi_event.data.control.value);
                set_program(midi_event.data.control.value);
                alsa_pcm.drain();
                break;
            default:
                SPDLOG_DEBUG("Engine MIDI event. [{}]", (int)midi_event.type);
                break;
        }
    }
}

void Engine::process_pcm() {
    TrackState* track;
    int ch;
    PcmSample_s24_3le* sample;

    while (state.push_pending()) {
        sample = state.frame.channels.data();
        track = state.tracks.data();
        while (track != state.tracks.end()) {
            ch = track->channels;
            while (ch-- > 0) {
                if (track->fifo != nullptr) {
                    while (!track->fifo->pop(*sample)) {
                        // TODO: Livelock? What if that track never gets data
                        usleep(500);
                    }
                } else {
                    sample->silence();
                }
                sample++;
#ifndef NDEBUG
                if (sample >= state.frame.channels.end()) {
                    throw new OstrostrojException("sample pointer overflow!");
                }
#endif
            }
            track++;
        }
        state.pending = true;
    }
}

Program& Engine::set_program(int program_number) {
    program = project.get_program(program_number);    
    for (std::unique_ptr<Track>& track : loop_tracks) {
        track->reset_node();
    }
    for (LoopClip& loop_clip : program.get().get_loops()) {
        loop_tracks.at(loop_clip.get_track())->set_node(std::make_unique<ClipNode>(loop_clip, true));
    }
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
    program(set_program(0)),
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