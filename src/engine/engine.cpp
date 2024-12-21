#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "engine.hpp"

static constexpr uint8_t SOURCE_MIDI_CHANNEL = 7;
static constexpr size_t MAX_MEM_BYTES = 7L*1024L*1024L*1024L;

bool Engine::handle_midi_event(snd_seq_event_t& midi_event, snd_pcm_state_t state, PcmEvent& result) {
    uint8_t mul;
    uint8_t clock_interval_ms;

    switch(midi_event.type) {
        case SND_SEQ_EVENT_START: 
            SPDLOG_INFO("ENGIN MIDI START [d0={},d1={},queue={}]", midi_event.data.queue.param.d32[0], midi_event.data.queue.param.d32[1],
                midi_event.data.queue.queue);
            if (state == SND_PCM_STATE_PREPARED) {
                session->start();
                result = ALSA_PCM_START;
                return true;
            }
            return false;
        case SND_SEQ_EVENT_STOP: 
            SPDLOG_INFO("ENGIN MIDI STOP [d0={},d1={},queue={}]", midi_event.data.queue.param.d32[0], midi_event.data.queue.param.d32[1],
                midi_event.data.queue.queue);
            result = ALSA_PCM_PAUSE;
            session->pause();
            return true;
        case SND_SEQ_EVENT_CONTINUE: 
            SPDLOG_INFO("ENGIN MIDI CONTINUE [d0={},d1={},queue={}]", midi_event.data.queue.param.d32[0], midi_event.data.queue.param.d32[1],
                midi_event.data.queue.queue);
            if (state == SND_PCM_STATE_PAUSED) {
                result = ALSA_PCM_RESUME;
                session->start();
                return true;
            }
            return false;
        case SND_SEQ_EVENT_PGMCHANGE:
            mul = midi_event.data.control.unused[0];
            clock_interval_ms = midi_event.data.control.unused[1];
            SPDLOG_INFO("ENGIN MIDI PROGRAM CHANGE [param={},value={},mul={},clock_interval_ms={}]", midi_event.data.control.param,
                midi_event.data.control.value, mul, clock_interval_ms);
            if (session->change_program(BankPattern(midi_event.data.control.value + 1), state == SND_PCM_STATE_RUNNING)) {
                program_changed(state == SND_PCM_STATE_RUNNING, compute_latency(mul, clock_interval_ms));
            }
            result = ALSA_PCM_PROGRAM_CHANGE;
            return true;
        case SND_SEQ_EVENT_NOTEON:
            note(midi_event.data.note.channel, midi_event.data.note.note, midi_event.data.note.velocity > 0,
                midi_event.time.tick, state == SND_PCM_STATE_RUNNING);
            return false;
        case SND_SEQ_EVENT_NOTEOFF:
            note(midi_event.data.note.channel, midi_event.data.note.note, false,
                midi_event.time.tick, state == SND_PCM_STATE_RUNNING);
            return false;
        case SND_SEQ_EVENT_CONTROLLER:
            controller(midi_event.data.control.channel, midi_event.data.control.param, midi_event.data.control.value,
                midi_event.time.tick, state == SND_PCM_STATE_RUNNING);
            return false;
        default:
            SPDLOG_WARN("ENGIN ignored engine MIDI event. [{}]", (int)midi_event.type);
            return false;
    }   
}

void Engine::note(uint8_t channel, uint8_t note, bool on, unsigned int clock, bool running) {
    switch (command_controller.note(channel, note, on, clock, running)) {
        case Command::RESTART_DEVICE:
            exit(EngineExit::ENGINE_EXIT_RESTART_DEVICE);
            return;
        case Command::SHUTDOWN_DEVICE:
            exit(EngineExit::ENGINE_EXIT_SHUTDOWN_DEVICE);
            return;
        case Command::RESTART_SERVICE:
            exit(EngineExit::ENGINE_EXIT_RESTART_SERVICE);
            return;
        case Command::NOOP:
            break;
    }
    if (channel != SOURCE_MIDI_CHANNEL || !running) {
        return;
    }
    if (pattern_learn->valid_note(note)) {
        if (!session->get_pattern().get_learned()) {
            pattern_learn->note(note, on, clock);
            session->step_learned();
            un_mute_mc_tracks();
        }
        return;
    }
    one_shot_note(note, on);
}

void Engine::exit(EngineExit _exit_code) {
    exit_code = _exit_code;
    SPDLOG_INFO("ENGIN exit[exit_code={}]", (int)exit_code);
    running_flag.clear();
    running_flag.notify_all();    
}

void Engine::one_shot_note(uint8_t note, bool on) {
    size_t octave = note / 12;
    // 4th octave + C5
    if (((octave != 4) && (note != 5*12)) || !on) {
        return;
    }
    note -= 4 * 12;
    size_t one_shot_number = INT_MAX;
    // The following logic is because how notes are layed out in two rows on Syntakt's keyboard (chromatic, folded)
    if (note < MainScreen::ONE_SHOT_COUNT / 2) {
        one_shot_number = 1 + note;
    } else {
        // Because Syntakt has 8 triggers in single row
        if (note >= 8) {
            one_shot_number = 1 + note - (8 - (MainScreen::ONE_SHOT_COUNT / 2));
        }
    }
    std::optional<std::reference_wrapper<SongOneShot>> one_shot_maybe = session->get_song().get_one_shot(one_shot_number);
    if (one_shot_maybe.has_value()) {
        lock_worker_tracks();
        one_shots_track.add_clip(session->get_clip(one_shot_maybe.value().get().one_shot), 0, false, false);
        unlock_worker_tracks();
        SPDLOG_DEBUG("ENGIN one shot added [one_shot_number={}]", one_shot_number);
        return;
    }
    lock_worker_tracks();
    one_shots_track.clear(false);
    unlock_worker_tracks();
    SPDLOG_DEBUG("ENGIN one shot not found [one_shot_number={}]", one_shot_number);
}

void Engine::controller(uint8_t channel, unsigned int param, signed int value, unsigned int clock, bool running) {
    if (channel != SOURCE_MIDI_CHANNEL || !running) {
        return;
    }
    if (pattern_learn->valid_controller(param) && !session->get_pattern().get_learned()) {
        learn(param, value, clock);
        return;
    }
    ssize_t track_number;
    if ((track_number = loop_encoders.handle(channel, param, value)) > 0) {
        update_saturation(track_number);
        return;
    }
}

void Engine::update_saturation(ssize_t track_number) {
    for (auto& track : loop_tracks) {
        if (track->get_track_number() == track_number) {
            MidiEncoder& encoder = loop_encoders.get_encoder(track_number);
            if (encoder.is_grabbed()) {
                track->set_saturation(encoder.get_percentage());
                PatternLoopSeq loop_seq = session->get_current_loop_seq(track_number);
                loop_seq.saturation = track->get_saturation();
                display.get_main_screen().set_loop_state(track_number - 1, loop_seq);
                display.get_main_screen().set_loop_grabbed(track_number - 1, true);
                display.tick(true);
            }
            return;
        }
    }
}

void Engine::learn(unsigned int param, signed int value, unsigned int clock) {
    bool had_one_shot_clip = session->get_current_one_shot_clip().has_value();
    pattern_learn->controller(param, value, clock);
    session->step_learned();            
    for (PatternLoop& pattern_loop : session->get_pattern().get_loops()) {
        PatternLoopSeq loop_seq = session->get_current_loop_seq(pattern_loop.track_number);
        if (!loop_seq.muted) {
            EngineWorker& worker = get_worker(pattern_loop.track_number);
            worker.lock_tracks();
            auto& track = loop_tracks.at(pattern_loop.track_number - 1);
            track->set_clip_mute(pattern_loop.loop, false);
            track->set_saturation(loop_seq.saturation);
            loop_encoders.get_encoder(track->get_track_number()).set_percentage(loop_seq.saturation);
            worker.unlock_tracks();
            SPDLOG_DEBUG("ENGIN loop unmuted [track_number={},loop={}]", pattern_loop.track_number, pattern_loop.loop.c_str());
        }
    }
    if (!had_one_shot_clip && session->get_current_one_shot_clip().has_value()) {
        // We just learned about one-shot, so let's add it
        EngineWorker& worker = get_worker(one_shots_track.get_track_number());
        worker.lock_tracks();
        add_one_shot_clip(last_computed_latency);
        worker.unlock_tracks();
        SPDLOG_DEBUG("ENGIN learned one-shot added");
    }
    SPDLOG_DEBUG("ENGIN controller learned [param={}]", param);
}

snd_pcm_uframes_t Engine::compute_latency(uint8_t mul, uint8_t clock_interval) {
    last_computed_latency = ((snd_pcm_uframes_t)mul * (snd_pcm_uframes_t)clock_interval * alsa_pcm.get_sample_rate()) / 1000;
    return last_computed_latency;
}

void Engine::program_changed(bool running, snd_pcm_uframes_t latency) {
    pattern_learn = std::make_unique<PatternLearn>(session->get_pattern());
    lock_worker_tracks();
    clear_loop_clips(running);
    add_loop_clips(running, latency);
    add_one_shot_clip(latency);
    unlock_worker_tracks();
    un_mute_mc_tracks();
    SPDLOG_INFO("ENGIN program set={}", session->get_pattern().get_bank_pattern().get_pattern());
}

void Engine::add_one_shot_clip(snd_pcm_uframes_t latency) {
    std::optional<std::reference_wrapper<Clip>> clip = session->get_current_one_shot_clip();
    if (clip.has_value()) {
        one_shots_track.add_clip(clip.value(), latency, true, false);
        SPDLOG_DEBUG("ENGIN one-shot added[clip={}]", clip.value().get().get_path().c_str());
    }
}

void Engine::add_loop_clips(bool running, snd_pcm_uframes_t latency) {
    for (PatternLoop& pattern_loop : session->get_pattern().get_loops()) {
        size_t track_index = pattern_loop.track_number - 1;
        if (track_index >= ENGINE_LOOP_TRACKS) {
            throw OstrostrojException(fmt::format("Invalid track index! [track_index={},loop={}]", track_index, pattern_loop.loop.filename().string()));
        }
        PatternLoopSeq loop_seq = session->get_current_loop_seq(pattern_loop.track_number);
        auto& track = loop_tracks.at(track_index);
        track->add_clip(session->get_clip(pattern_loop.loop), latency, running, loop_seq.muted);
        track->set_saturation(loop_seq.saturation);
        loop_encoders.get_encoder(track->get_track_number()).set_percentage(loop_seq.saturation);
        SPDLOG_DEBUG("ENGIN clip added [track_index={},loop={},muted={},saturation={}]", track_index, pattern_loop.loop.c_str(), loop_seq.muted, loop_seq.saturation);
    }
}

void Engine::clear_loop_clips(bool running) {
    for (size_t i = 0; i < loop_tracks.size(); i++) {
        loop_tracks.at(i)->clear(!running);
    }
    one_shots_track.clear(!running);
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

void Engine::un_mute_mc_tracks() {
    for (size_t mc_track_number = 1; mc_track_number <= ModelCycles::MODEL_CYCLES_TRACK_COUNT; mc_track_number++) {
        snd_seq_event_t event = model_cycles->mute_track(mc_track_number, session->get_current_mute(mc_track_number));
        alsa_midi.write(event);
    }
}

EngineWorker& Engine::get_worker(uint8_t track_number) {
    for (std::unique_ptr<EngineWorker>& worker: workers) {
        for (Track& track : worker->get_tracks()) {
            if (track.get_track_number() == track_number) {
                return *worker;
            }
        }
    }
    throw OstrostrojException(fmt::format("ENGIN worker not found! [track_number={}]", track_number));
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

void Engine::init_display() {
    size_t mem_size_bytes = session->get_mem_size_bytes();
    float mem_usage = (float)mem_size_bytes / (float)MAX_MEM_BYTES;
    SPDLOG_INFO("ENGIN memory used={} MB ({:.1f}%)", mem_size_bytes/(1024*1024), mem_usage);
    display.get_system_screen().set_mem_usage(mem_usage);
    display.tick(true);
    sleep(1);
    display.set_active_screen(display.get_main_screen());
    display.tick(true);
}

void Engine::midi_callback() {
    midi_flag.clear();
    midi_flag.notify_one();
}

Engine::Engine(Project& _project, AlsaPcm& _alsa_pcm, Display& _display, std::atomic_flag& _running_flag):
    alsa_midi(),
    alsa_pcm(_alsa_pcm),
    display(_display),
    running_flag(_running_flag),
    model_cycles(std::make_unique<ModelCycles>()),
    loop_encoders(SOURCE_MIDI_CHANNEL, L1_PARAM),
    command_controller(SOURCE_MIDI_CHANNEL),
    midi_flag(ATOMIC_FLAG_INIT),
    stop(false),
    loop_tracks {
        std::make_unique<Track>(1, 1, _alsa_pcm.get_period_size(), _alsa_pcm.get_periods(), true),
        std::make_unique<Track>(2, 1, _alsa_pcm.get_period_size(), _alsa_pcm.get_periods(), true),
        std::make_unique<Track>(3, 1, _alsa_pcm.get_period_size(), _alsa_pcm.get_periods(), true),
        std::make_unique<Track>(4, 1, _alsa_pcm.get_period_size(), _alsa_pcm.get_periods(), true),
        std::make_unique<Track>(5, 1, _alsa_pcm.get_period_size(), _alsa_pcm.get_periods(), true),
        std::make_unique<Track>(6, 2, _alsa_pcm.get_period_size(), _alsa_pcm.get_periods(), true),
    },
    one_shots_track(Track(7, 2, _alsa_pcm.get_period_size(), _alsa_pcm.get_periods(), false)),
    track_fifos(init_track_fifos()),
    worker_sleep_time(std::chrono::microseconds(_alsa_pcm.get_period_time()).count() / 2),
    workers(create_workers()),
    pattern_learn(),
    exit_code(EngineExit::ENGINE_EXIT_NOOP),
    last_computed_latency(0) {
    session = std::make_unique<Session>(_project, display, alsa_pcm.get_sample_rate(), loop_tracks.size());
    init_display();
    alsa_midi.start(std::bind(&Engine::midi_callback, this));
}

Engine::~Engine() {
}

void Engine::shutdown() {
    workers.clear();
    stop = true;
    midi_callback();
}

bool Engine::pcm_event_callback(PcmEvent& event, snd_pcm_state_t state, bool sync) {
    if (stop) {
        return false;
    }
    snd_seq_event_t midi_event;
    if (alsa_midi.get_fifo_in().pop(midi_event)) {
        return handle_midi_event(midi_event, state, event);
    } else {
        if (sync) {
            midi_flag.test_and_set();
            SPDLOG_TRACE("ENGIN waiting for MIDI flag...");
            midi_flag.wait(true);
            if (!alsa_midi.get_fifo_in().pop(midi_event)) {
                if (stop) {
                    return false;
                }
                throw OstrostrojException("ENGIN MIDI FIFO empty!");
            }
            SPDLOG_TRACE("ENGIN waiting for MIDI flag done.");
            return handle_midi_event(midi_event, state, event);
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
                SPDLOG_DEBUG("ENGIN track FIFO {} underrun...", sample - frame.channels.data());
                int retries = 0;
                do {
                    usleep(worker_sleep_time);
                    retries++;
                } while (!(*track_fifo)->pop(*sample) && (!stop));
                SPDLOG_DEBUG("ENGIN track FIFO {} underrun recovered [retries={}]", sample - frame.channels.data(), retries);
            }
        }
        sample++;
        track_fifo++;
    }
    session->draw();
}

int Engine::get_loop_track_count() const {
    return loop_tracks.size();    
}

EngineExit Engine::get_exit_code() const {
    return exit_code;
}