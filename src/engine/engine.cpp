#define SPDLOG_ACTIVE_LEVEL 2

#include "common.hpp"
#include "engine.hpp"

bool Engine::handle_midi_event(snd_seq_event_t& midi_event, bool running, PcmEvent& result) {
    uint8_t mul;
    uint8_t clock_interval_ms;

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
            mul = midi_event.data.control.unused[0];
            clock_interval_ms = midi_event.data.control.unused[1];
            SPDLOG_INFO("ENGIN MIDI PROGRAM CHANGE [param={},value={},mul={},clock_interval_ms={}]", midi_event.data.control.param,
                midi_event.data.control.value, mul, clock_interval_ms);
            if (session->change_program(BankPattern(midi_event.data.control.value + 1))) {
                program_changed(running, compute_predelay(mul, clock_interval_ms));
            }
            result = ALSA_PCM_PROGRAM_CHANGE;
            return true;
        case SND_SEQ_EVENT_NOTEON:
            pattern_learn->note(midi_event.data.note.note, midi_event.data.note.velocity > 0, midi_event.time.tick);
            return false;
        case SND_SEQ_EVENT_CONTROLLER:
            pattern_learn->controller(midi_event.data.control.param, midi_event.data.control.value, midi_event.time.tick);
            return false;
        default:
            SPDLOG_WARN("ENGIN ignored engine MIDI event. [{}]", (int)midi_event.type);
            return false;
    }   
}

snd_pcm_uframes_t Engine::compute_predelay(uint8_t mul, uint8_t clock_interval) {
    return ((snd_pcm_uframes_t)mul * (snd_pcm_uframes_t)clock_interval * alsa_pcm.get_sample_rate()) / 1000;
}

void Engine::program_changed(bool running, snd_pcm_uframes_t predelay) {
    pattern_learn = std::make_unique<PatternLearn>(session->get_pattern());
    lock_worker_tracks();
    clear_loop_clips(running);
    add_loop_clips(running ? predelay : 0);
    unlock_worker_tracks();
    SPDLOG_INFO("ENGIN program set={}", session->get_pattern().get_bank_pattern().get_pattern());
}

void Engine::add_loop_clips(snd_pcm_uframes_t predelay) {
    for (PatternLoop& pattern_loop : session->get_pattern().get_loops()) {
        size_t track_index = pattern_loop.track - 1;
        if (track_index >= ENGINE_LOOP_TRACKS) {
            throw OstrostrojException(fmt::format("Invalid track index! [track_index={},loop={}]", track_index, pattern_loop.loop.filename().string()));
        }
        loop_tracks.at(track_index)->add_clip(session->get_clip(pattern_loop.loop), predelay);
        SPDLOG_DEBUG("ENGIN clip added [track_index={},loop={}]", track_index, pattern_loop.loop.c_str());
    }
}

void Engine::clear_loop_clips(bool running) {
    for (size_t i = 0; i < loop_tracks.size(); i++) {
        loop_tracks.at(i)->clear(!running);
    }
}

void Engine::lock_worker_tracks() {
    for (std::unique_ptr<EngineWorker>& worker: workers) {
        worker->lock_tracks();
    }
}

void Engine::unlock_worker_tracks() {
    for (std::unique_ptr<EngineWorker>& worker: workers) {
        worker->unlock_tracks();
    }
}

void Engine::add_worker_track(std::map<size_t, std::vector<std::reference_wrapper<Track>>>& worker_tracks, Track& track) {
    size_t min = ULONG_MAX;
    size_t worker = 0;
    for (size_t i = 0; i < worker_tracks.size(); i++) {
        size_t worker_size = 0;
        std::vector<std::reference_wrapper<Track>> tracks = worker_tracks.at(i);
        for (std::reference_wrapper<Track>& t: tracks) {
            worker_size += t.get().get_channels();
        }
        if (worker_size < min) {
            min = worker_size;
            worker = i;
        }
    }
    worker_tracks.at(worker).emplace_back(std::ref(track));
}

std::vector<std::unique_ptr<EngineWorker>> Engine::create_workers() {
    std::map<size_t, std::vector<std::reference_wrapper<Track>>> worker_tracks;
    for (size_t i = 0; i < std::thread::hardware_concurrency(); i++) {
        worker_tracks.emplace(i, std::vector<std::reference_wrapper<Track>>());
    }

    // Assign stereo tracks
    for (size_t stereo_loop_track = ENGINE_LOOP_MONO_TRACKS; stereo_loop_track < loop_tracks.size(); stereo_loop_track++) {
        add_worker_track(worker_tracks, *loop_tracks.at(stereo_loop_track));
    }

    // Assign mono tracks
    for (size_t mono_loop_track = 0; mono_loop_track < ENGINE_LOOP_MONO_TRACKS; mono_loop_track++) {
        add_worker_track(worker_tracks, *loop_tracks.at(mono_loop_track));
    }

    // Assign one-shots track (stereo)
    add_worker_track(worker_tracks, one_shots_track);

    std::vector<std::unique_ptr<EngineWorker>> result;
    for (size_t i = 0; i < std::thread::hardware_concurrency(); i++) {
        result.emplace_back(std::make_unique<EngineWorker>(worker_tracks.at(i), i, worker_sleep_time));
    }
    return result;
}

std::array<InterleavedFifo*, PCM_OUT_CHANNELS> Engine::init_track_fifos() {    
    std::array<InterleavedFifo*, PCM_OUT_CHANNELS> result;
    result.fill(nullptr);

    // Loop tracks
    size_t fifo_index = 0;
    for (size_t track_index = 0; track_index < loop_tracks.size(); track_index++) {
        Track& loop_track = *loop_tracks.at(track_index);
        InterleavedFifo& loop_track_fifo = loop_track.get_fifo();
        if (track_index < ENGINE_LOOP_MONO_TRACKS) {
            assert(loop_track.get_channels() == 1);
            result.at(fifo_index++) = &loop_track_fifo;
        } else {
            // Stereo tracks are interleaved
            assert(loop_track.get_channels() == 2);
            result.at(fifo_index++) = &loop_track_fifo;
            result.at(fifo_index++) = &loop_track_fifo;
        }
    }

    // One-shots track is stereo interleaved
    result.at(fifo_index++) = &one_shots_track.get_fifo();
    result.at(fifo_index++) = &one_shots_track.get_fifo();

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
        std::make_unique<Track>(1, 1, _alsa_pcm.get_period_size(), _alsa_pcm.get_periods(), true),
        std::make_unique<Track>(2, 1, _alsa_pcm.get_period_size(), _alsa_pcm.get_periods(), true),
        std::make_unique<Track>(3, 1, _alsa_pcm.get_period_size(), _alsa_pcm.get_periods(), true),
        std::make_unique<Track>(4, 1, _alsa_pcm.get_period_size(), _alsa_pcm.get_periods(), true),
        std::make_unique<Track>(5, 2, _alsa_pcm.get_period_size(), _alsa_pcm.get_periods(), true),
        std::make_unique<Track>(6, 2, _alsa_pcm.get_period_size(), _alsa_pcm.get_periods(), true),
    },
    one_shots_track(Track(7, 2, _alsa_pcm.get_period_size(), _alsa_pcm.get_periods(), false)),
    track_fifos(init_track_fifos()),
    worker_sleep_time(std::chrono::microseconds(_alsa_pcm.get_period_time()).count() / 2),
    workers(create_workers()) {
    // Initialize session
    session = std::make_unique<Session>(workspace.get_projects().at(0), display, alsa_pcm.get_sample_rate(), loop_tracks.size());

    // Initialize
    program_changed(false, 0);
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