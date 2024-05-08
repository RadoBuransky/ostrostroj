#include "common.hpp"
#include "engine.hpp"

void Engine::run() {
    try {
        PcmFrame_s24_3le frame;
        frame.silence();
        SPDLOG_INFO("Engine started.");
        while (!stop) {
            process_midi();
            process_pcm(frame);
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
                break;
            case SND_SEQ_EVENT_STOP:
                break;
            case SND_SEQ_EVENT_CONTINUE:
                break;
            case SND_SEQ_EVENT_PGMCHANGE:
                set_program(midi_event.data.control.value);
                break;
        }
    }
}

void Engine::process_pcm(PcmFrame_s24_3le& frame) {
    PcmFifo& pcm_fifo = alsa_pcm.get_pcm_fifo();
    while(pcm_fifo.push(std::move(frame))) {
        PcmSample_s24_3le* frame_channel = frame.channels.data();
        for (size_t loop_track_index = 0; loop_track_index < loop_tracks.size(); loop_track_index++) {
            InterleavedFifo& loop_track_fifo = loop_tracks[loop_track_index]->get_fifo();
            for (int channel = 0; channel < loop_tracks[loop_track_index]->get_channels(); channel++) {
                if (!loop_track_fifo.pop(*frame_channel)) {                
                    frame_channel->silence();
                }
                frame_channel++;
            }
        }
        if (!one_shots_track.get_fifo().pop(*frame_channel)) {
            frame_channel->silence();
        }
        frame_channel++;
        while (frame_channel < frame.channels.end()) {
            frame_channel->silence();
            frame_channel++;
        }
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
        std::make_unique<Track>(1, 1, _alsa_pcm.get_period_time(), _alsa_pcm.get_period_size()),
        std::make_unique<Track>(2, 1, _alsa_pcm.get_period_time(), _alsa_pcm.get_period_size()),
        std::make_unique<Track>(3, 1, _alsa_pcm.get_period_time(), _alsa_pcm.get_period_size()),
        std::make_unique<Track>(4, 1, _alsa_pcm.get_period_time(), _alsa_pcm.get_period_size()),
        std::make_unique<Track>(5, 2, _alsa_pcm.get_period_time(), _alsa_pcm.get_period_size()),
        std::make_unique<Track>(6, 2, _alsa_pcm.get_period_time(), _alsa_pcm.get_period_size()),
    },
    one_shots_track(Track(7, 2, _alsa_pcm.get_period_time(), _alsa_pcm.get_period_size())),
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
    return 0;    
}

void Engine::pcm_callback() {
    heartbeat.clear();
    heartbeat.notify_one();
}

void Engine::midi_callback() {
    heartbeat.clear();
    heartbeat.notify_one();
}