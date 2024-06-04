#define SPDLOG_ACTIVE_LEVEL 2

#include "common.hpp"
#include "engine.hpp"

bool Engine::handle_midi_event(snd_seq_event_t& midi_event, bool running, PcmEvent& result) {
    switch(midi_event.type) {
        case SND_SEQ_EVENT_START: 
            SPDLOG_INFO("ENGIN MIDI START [d0={},d1={},queue={}]", midi_event.data.queue.param.d32[0], midi_event.data.queue.param.d32[1],
                midi_event.data.queue.queue);
            result = ALSA_PCM_START;
            session->start();
            return true;
        case SND_SEQ_EVENT_STOP: 
            SPDLOG_INFO("ENGIN MIDI STOP [d0={},d1={},queue={}]", midi_event.data.queue.param.d32[0], midi_event.data.queue.param.d32[1],
                midi_event.data.queue.queue);
            result = ALSA_PCM_PAUSE;
            session->pause();
            return true;
        case SND_SEQ_EVENT_CONTINUE: 
            SPDLOG_INFO("ENGIN MIDI CONTINUE [d0={},d1={},queue={}]", midi_event.data.queue.param.d32[0], midi_event.data.queue.param.d32[1],
                midi_event.data.queue.queue);
            result = ALSA_PCM_RESUME;
            session->start();
            return true;
        case SND_SEQ_EVENT_PGMCHANGE: 
            SPDLOG_INFO("ENGIN MIDI PROGRAM CHANGE [param={},value={}]", midi_event.data.control.param, midi_event.data.control.value);
            if (session->change_program(BankPattern(midi_event.data.control.value + 1))) {
                program_changed(running);
            }
            result = ALSA_PCM_PROGRAM_CHANGE;
            return true;
        case SND_SEQ_EVENT_NOTEON:
            pattern_learn->note(midi_event.data.note.note, midi_event.data.note.velocity > 0, midi_event.time.tick);
            return true;
        case SND_SEQ_EVENT_CONTROLLER:
            pattern_learn->controller(midi_event.data.control.param, midi_event.data.control.value, midi_event.time.tick);
            return true;
        default:
            SPDLOG_WARN("ENGIN ignored engine MIDI event. [{}]", (int)midi_event.type);
            return false;
    }   
}

void Engine::program_changed(bool running) {
    pattern_learn = std::make_unique<PatternLearn>(session->get_pattern());
    release_worker_tracks();
    reset_program(running);
    update_tracks();
    assign_worker_tracks();
    SPDLOG_INFO("ENGIN program set={}", session->get_pattern().get_bank_pattern().get_pattern());
}

void Engine::update_tracks() {
    track_fifos.fill(nullptr);
    for (PatternLoop& pattern_loop : session->get_pattern().get_loops()) {
        size_t track_index = pattern_loop.track - 1;
        if (track_index >= ENGINE_LOOP_TRACKS) {
            throw OstrostrojException(fmt::format("Invalid track index! [track_index={},loop={}]", track_index, pattern_loop.loop.filename().string()));
        }
        Track& loop_track = *loop_tracks.at(track_index);
        InterleavedFifo& loop_track_fifo = loop_track.get_fifo();
        loop_track.add_clip(session->get_clip(pattern_loop.loop));
        if (track_index < ENGINE_LOOP_MONO_TRACKS) {
            assert(loop_track.get_channels() == 1);
            track_fifos.at(track_index) = &loop_track_fifo;
        } else {
            // Stereo tracks are interleaved
            assert(loop_track.get_channels() == 2);
            track_fifos.at(ENGINE_LOOP_MONO_TRACKS + (track_index - ENGINE_LOOP_MONO_TRACKS) * 2) = &loop_track_fifo;
            track_fifos.at(ENGINE_LOOP_MONO_TRACKS + (track_index - ENGINE_LOOP_MONO_TRACKS) * 2 + 1) = &loop_track_fifo;
        }
    }
}

void Engine::reset_program(bool running) {
    for (size_t i = 0; i < loop_tracks.size(); i++) {
        loop_tracks.at(i)->clear(!running);
    }
}

void Engine::assign_worker_tracks() {
    std::vector<std::reference_wrapper<Track>> all_tracks;
    for (PatternLoop& pattern_loop : session->get_pattern().get_loops()) {
        all_tracks.push_back(std::ref(*loop_tracks.at(pattern_loop.track - 1)));
    }
    all_tracks.push_back(std::ref(one_shots_track));

    for (size_t worker_index = 0; worker_index < workers.size(); worker_index++) {
        std::vector<std::reference_wrapper<Track>> worker_tracks;
        size_t track_index = worker_index;
        while (track_index < all_tracks.size()) {
            worker_tracks.push_back(std::ref(all_tracks.at(track_index)));
            track_index += workers.size();
        }
        workers.at(worker_index)->assign_tracks(worker_tracks);
    }
}

// Call these before touching Tracks and Nodes, they are not thread safe
void Engine::release_worker_tracks() {
    for (std::unique_ptr<EngineWorker>& worker: workers) {
        worker->release_tracks();
    }
}

std::vector<std::unique_ptr<EngineWorker>> Engine::create_workers() {
    std::vector<std::unique_ptr<EngineWorker>> result;
    for (size_t i = 0; i < std::thread::hardware_concurrency(); i++) {
        result.emplace_back(std::make_unique<EngineWorker>(i, worker_sleep_time));
    }
    return result;
}

Engine::Engine(Workspace& _workspace, AlsaMidi& _alsa_midi, AlsaPcm& _alsa_pcm, Display& _display):
    workspace(_workspace),
    alsa_midi(_alsa_midi),
    alsa_pcm(_alsa_pcm),
    display(_display),
    midi_flag(ATOMIC_FLAG_INIT),
    stop(false),
    loop_tracks {
        std::make_unique<Track>(1, 1, _alsa_pcm.get_period_size(), true),
        std::make_unique<Track>(2, 1, _alsa_pcm.get_period_size(), true),
        std::make_unique<Track>(3, 1, _alsa_pcm.get_period_size(), true),
        std::make_unique<Track>(4, 1, _alsa_pcm.get_period_size(), true),
        std::make_unique<Track>(5, 2, _alsa_pcm.get_period_size(), true),
        std::make_unique<Track>(6, 2, _alsa_pcm.get_period_size(), true),
    },
    one_shots_track(Track(7, 2, _alsa_pcm.get_period_size(), false)),
    worker_sleep_time(std::chrono::microseconds(_alsa_pcm.get_period_time()).count() / 2),
    workers(create_workers()) {
    // Initialize session
    session = std::make_unique<Session>(workspace.get_projects().at(0), display, alsa_pcm.get_sample_rate(), loop_tracks.size());

    // One-shots track is stereo interleaved
    track_fifos.at(ENGINE_LOOP_TRACKS) = &one_shots_track.get_fifo();
    track_fifos.at(ENGINE_LOOP_TRACKS + 1) = &one_shots_track.get_fifo();

    // Initialize
    program_changed(false);
    display.tick(true);
}

Engine::~Engine() {
}

void Engine::shutdown() {
    workers.clear();
    stop = true;
    midi_callback();
}

bool Engine::pcm_event_callback(PcmEvent& event, bool running, bool sync) {
    if (stop) {
        return false;
    }
    snd_seq_event_t midi_event;
    if (alsa_midi.get_fifo().pop(midi_event)) {
        return handle_midi_event(midi_event, running, event);
    } else {
        if (sync) {
            midi_flag.test_and_set();
            SPDLOG_TRACE("ENGIN waiting for MIDI flag...");
            midi_flag.wait(true);
            if (!alsa_midi.get_fifo().pop(midi_event)) {
                if (stop) {
                    return false;
                }
                throw OstrostrojException("ENGIN MIDI FIFO empty!");
            }
            SPDLOG_TRACE("ENGIN waiting for MIDI flag done.");
            return handle_midi_event(midi_event, running, event);
        }
        return false;
    }
}

void Engine::pcm_callback(PcmFrame_s24_3le& frame) {
    PcmSample_s24_3le* sample = frame.channels.data();
    InterleavedFifo** track_fifo = track_fifos.data();
    while (sample != frame.channels.end() && track_fifo != track_fifos.end()) {
        if (*track_fifo == nullptr) {
            sample->silence();
        } else {
            if (!(*track_fifo)->pop(*sample)) {
                SPDLOG_WARN("ENGIN track FIFO {} underrun...", sample - frame.channels.data());
                int retries = 0;
                do {
                    usleep(worker_sleep_time);
                    retries++;
                } while (!(*track_fifo)->pop(*sample) && (!stop));
                SPDLOG_WARN("ENGIN track FIFO {} underrun recovered [retries={}]", sample - frame.channels.data(), retries);
            }
        }
        sample++;
        track_fifo++;
    }
    session->draw();
}

void Engine::midi_callback() {
    midi_flag.clear();
    midi_flag.notify_one();
}

int Engine::get_loop_track_count() const {
    return loop_tracks.size();    
}