#define SPDLOG_ACTIVE_LEVEL 2

#include "common.hpp"
#include "engine.hpp"

void EngineWorker::run() {
    try {
        SPDLOG_INFO("Engine worker started. [{},sleep_time={}]", worker_index, sleep_time);
        while (!stop) {
            if (run_tracks()) {
                usleep(sleep_time);
            }
        }
        SPDLOG_INFO("Engine worker stopped. [{}]", worker_index);
    } catch(std::exception const& e) {
        SPDLOG_ERROR("Engine worker {} failed. {}", worker_index, e.what());
    }
}

bool EngineWorker::run_tracks() {
    std::unique_lock lock(m);
    if (tracks.empty()) {
        SPDLOG_INFO("Engine worker {} waiting for tracks...", worker_index);
        cv.wait(lock, [&]{ return tracks.empty(); });
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
    thread(std::bind(&EngineWorker::run, this)) {    
    pthread_setname_np(thread.native_handle(), fmt::format("worker{}", _worker_index).c_str());
}

EngineWorker::~EngineWorker() {
    stop = true;
    thread.join();
}

void EngineWorker::set_tracks(std::vector<std::reference_wrapper<Track>> _tracks) {
    std::lock_guard lock(m);
    tracks = _tracks;
    cv.notify_one();
    SPDLOG_INFO("Engine worker tracks set. [size={}]", tracks.size());
}

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

            // TODO: Mutex
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
    for (size_t track = 0; track < loop_tracks.size(); track++) {
        loop_tracks.at(track)->reset_node();
    }
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
    // One-shots track is stereo interleaved
    track_fifos.at(ENGINE_LOOP_TRACKS) = &one_shots_track.get_fifo();
    track_fifos.at(ENGINE_LOOP_TRACKS + 1) = &one_shots_track.get_fifo();
    update_worker_tracks();
    SPDLOG_INFO("Program set = {}", program.get().get_start_number());
    return program;
}

void Engine::update_worker_tracks() {
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
        workers.at(worker_index)->set_tracks(worker_tracks);
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
    program(set_program(1)),
    worker_sleep_time(std::chrono::microseconds(_alsa_pcm.get_period_time()).count() / 2),
    workers(create_workers()) {
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

void Engine::pcm_callback(PcmFrame_s24_3le& frame) {
    PcmSample_s24_3le* sample = frame.channels.data();
    InterleavedFifo** track_fifo = track_fifos.data();
    while (sample != frame.channels.end() && track_fifo != track_fifos.end()) {
        if (*track_fifo == nullptr) {
            sample->silence();
        } else {
            if (!(*track_fifo)->pop(*sample)) {
                SPDLOG_WARN("Engine track FIFO {} underrun...", sample - frame.channels.data());
                int retries = 0;
                do {
                    usleep(worker_sleep_time);
                    retries++;
                } while (!(*track_fifo)->pop(*sample));
                SPDLOG_WARN("Engine track FIFO {} underrun recovered [retries={}]", sample - frame.channels.data(), retries);
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