#define SPDLOG_ACTIVE_LEVEL 2

#include "common.hpp"
#include "engine.hpp"

void EngineWorker::run() {
    try {
        SPDLOG_INFO("EW{}   started [sleep_time={}us]", worker_index, sleep_time);
        while (!stop) {
            if (run_tracks()) {
                usleep(sleep_time);
            }
        }
        SPDLOG_INFO("EW{}   stopped", worker_index);
    } catch(std::exception const& e) {
        SPDLOG_ERROR("EW{}   failed. {}", worker_index, e.what());
    }
}

bool EngineWorker::run_tracks() {
    std::unique_lock lock(mutex);
    if (tracks.empty()) {
        SPDLOG_DEBUG("EW{}   waiting for tracks...", worker_index);
        cv.wait(lock, [&]{ return !tracks.empty(); });
        SPDLOG_DEBUG("EW{}   waiting done. [tracks={}]", worker_index, tracks.size());
        return false;
    }
    for (std::reference_wrapper<Track> track: tracks) {
        track.get().run();
    }
    return true;
}

EngineWorker::EngineWorker(int _worker_index, useconds_t _sleep_time):
    worker_index(_worker_index),
    sleep_time(_sleep_time),
    stop(false),
    mutex(),
    cv(),
    tracks(),
    thread(std::bind(&EngineWorker::run, this)) {    
    pthread_setname_np(thread.native_handle(), fmt::format("worker{}", _worker_index).c_str());
}

EngineWorker::~EngineWorker() {
    stop = true;
    thread.join();
}

void EngineWorker::assign_tracks(std::vector<std::reference_wrapper<Track>> _tracks) {
    std::lock_guard lock(mutex);
    tracks = _tracks;
    cv.notify_one();
    SPDLOG_INFO("EW{}   tracks set. [size={}]", worker_index, tracks.size());
}

void EngineWorker::release_tracks() {
    std::lock_guard lock(mutex);
    tracks.clear();
}

bool Engine::handle_midi_event(snd_seq_event_t& midi_event, bool running, PcmEvent& result) {
    switch(midi_event.type) {
        case SND_SEQ_EVENT_START: 
            SPDLOG_INFO("ENGIN MIDI START [d0={},d1={},queue={}]", midi_event.data.queue.param.d32[0], midi_event.data.queue.param.d32[1],
                midi_event.data.queue.queue);
            result = ALSA_PCM_START;
            return true;
        case SND_SEQ_EVENT_STOP: 
            SPDLOG_INFO("ENGIN MIDI STOP [d0={},d1={},queue={}]", midi_event.data.queue.param.d32[0], midi_event.data.queue.param.d32[1],
                midi_event.data.queue.queue);
            result = ALSA_PCM_PAUSE;
            return true;
        case SND_SEQ_EVENT_CONTINUE: 
            SPDLOG_INFO("ENGIN MIDI CONTINUE [d0={},d1={},queue={}]", midi_event.data.queue.param.d32[0], midi_event.data.queue.param.d32[1],
                midi_event.data.queue.queue);
            result = ALSA_PCM_RESUME;
            return true;
        case SND_SEQ_EVENT_PGMCHANGE: 
            SPDLOG_INFO("ENGIN MIDI PROGRAM CHANGE [param={},value={}]", midi_event.data.control.param, midi_event.data.control.value);
            // TODO: xfade tracks
            change_program(midi_event.data.control.value + 1, running);
            result = ALSA_PCM_PROGRAM_CHANGE;
            return true;
        default:
            SPDLOG_WARN("ENGIN ignored engine MIDI event. [{}]", (int)midi_event.type);
            return false;
    }   
}

void Engine::change_program(int program_number, bool running) {
    program = project.get_program(program_number);
    release_worker_tracks();
    reset_program(running);
    update_tracks();
    assign_worker_tracks();
    SPDLOG_INFO("ENGIN program set={}", program.get().get_start_number());
}

void Engine::update_tracks() {
    track_fifos.fill(nullptr);
    for (LoopClip& loop_clip : program.get().get_loops()) {
        int track = loop_clip.get_track();
        assert(track < ENGINE_LOOP_TRACKS);
        Track& loop_track = *loop_tracks.at(track);
        InterleavedFifo& loop_track_fifo = loop_track.get_fifo();
        loop_track.set_node(std::make_unique<ClipNode>(loop_clip, true));
        if (track < ENGINE_LOOP_MONO_TRACKS) {
            assert(loop_track.get_channels() == 1);
            track_fifos.at(track) = &loop_track_fifo;
        } else {
            // Stereo tracks are interleaved
            assert(loop_track.get_channels() == 2);
            track_fifos.at(ENGINE_LOOP_MONO_TRACKS + (track - ENGINE_LOOP_MONO_TRACKS) * 2) = &loop_track_fifo;
            track_fifos.at(ENGINE_LOOP_MONO_TRACKS + (track - ENGINE_LOOP_MONO_TRACKS) * 2 + 1) = &loop_track_fifo;
        }
    }
}

void Engine::reset_program(bool running) {
    for (size_t i = 0; i < loop_tracks.size(); i++) {
        loop_tracks.at(i)->reset_node();
        if (!running) {
            loop_tracks.at(i)->drop();
        }
    }
}

void Engine::assign_worker_tracks() {
    std::vector<std::reference_wrapper<Track>> all_tracks;
    for (LoopClip& loop_clip : program.get().get_loops()) {
        all_tracks.push_back(std::ref(*loop_tracks.at(loop_clip.get_track())));
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
    program(project.get_program(1)),
    worker_sleep_time(std::chrono::microseconds(_alsa_pcm.get_period_time()).count() / 2),
    workers(create_workers()) {
    // One-shots track is stereo interleaved
    track_fifos.at(ENGINE_LOOP_TRACKS) = &one_shots_track.get_fifo();
    track_fifos.at(ENGINE_LOOP_TRACKS + 1) = &one_shots_track.get_fifo();

    // Initialize
    change_program(1, false);
}

Engine::~Engine() {
}

bool Engine::pcm_event_callback(PcmEvent& event, bool running, bool sync) {
    snd_seq_event_t midi_event;
    if (alsa_midi.get_fifo().pop(midi_event)) {
        return handle_midi_event(midi_event, running, event);
    } else {
        if (sync) {
            midi_flag.test_and_set();
            midi_flag.wait(true);
            if (!alsa_midi.get_fifo().pop(midi_event)) {
                throw OstrostrojException("ENGIN MIDI FIFO empty!");
            }
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
                } while (!(*track_fifo)->pop(*sample));
                SPDLOG_WARN("ENGIN track FIFO {} underrun recovered [retries={}]", sample - frame.channels.data(), retries);
            }
        }
        sample++;
        track_fifo++;
    }
}

void Engine::midi_callback() {
    midi_flag.clear();
    midi_flag.notify_one();
}

int Engine::get_loop_track_count() const {
    return loop_tracks.size();    
}