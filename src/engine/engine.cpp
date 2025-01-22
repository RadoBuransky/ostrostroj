#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 1
#include <spdlog/spdlog.h>
#include "engine.hpp"

static constexpr uint8_t SOURCE_MIDI_CHANNEL = 7;
static constexpr size_t MAX_MEM_BYTES = 7L*1024L*1024L*1024L;

bool Engine::handle_midi_event(snd_seq_event_t& midi_event, snd_pcm_state_t state, PcmEvent& result) {
    uint8_t mul;
    uint8_t clock_interval_ms;

    switch(midi_event.type) {
        case SND_SEQ_EVENT_START: 
            SPDLOG_INFO("ENGIN MIDI START [state={},d0={},d1={},queue={}]", (int)state, midi_event.data.queue.param.d32[0], midi_event.data.queue.param.d32[1],
                midi_event.data.queue.queue);
            if (state == SND_PCM_STATE_PREPARED) {
                session.start();
                result = ALSA_PCM_START;
                SPDLOG_DEBUG("ENGIN SND_PCM_STATE_PREPARED");
                return true;
            }
            return false;
        case SND_SEQ_EVENT_STOP: 
            SPDLOG_INFO("ENGIN MIDI STOP [d0={},d1={},queue={}]", midi_event.data.queue.param.d32[0], midi_event.data.queue.param.d32[1],
                midi_event.data.queue.queue);
            result = ALSA_PCM_PAUSE;
            session.pause();
            return true;
        case SND_SEQ_EVENT_CONTINUE: 
            SPDLOG_INFO("ENGIN MIDI CONTINUE [d0={},d1={},queue={}]", midi_event.data.queue.param.d32[0], midi_event.data.queue.param.d32[1],
                midi_event.data.queue.queue);
            if (state == SND_PCM_STATE_PAUSED) {
                result = ALSA_PCM_RESUME;
                session.start();
                return true;
            }
            return false;
        case SND_SEQ_EVENT_PGMCHANGE:
            SPDLOG_INFO("ENGIN MIDI PROGRAM CHANGE [param={},value={},mul={},clock_interval_ms={}]", midi_event.data.control.param,
                midi_event.data.control.value, mul, clock_interval_ms);
            if (change_program(state == SND_PCM_STATE_RUNNING, BankPattern(midi_event.data.control.value + 1))) {
                result = ALSA_PCM_PROGRAM_CHANGE;
                return true;
            }
            return false;
        case SND_SEQ_EVENT_NOTEON:
            note(midi_event.data.note.channel, midi_event.data.note.note, midi_event.data.note.velocity > 0,
                midi_event.time.tick, state == SND_PCM_STATE_RUNNING);
            return false;
        case SND_SEQ_EVENT_NOTEOFF:
            note(midi_event.data.note.channel, midi_event.data.note.note, false,
                midi_event.time.tick, state == SND_PCM_STATE_RUNNING);
            return false;
        case SND_SEQ_EVENT_CONTROLLER:
            on_controller(midi_event.data.control.channel, midi_event.data.control.param, midi_event.data.control.value, state == SND_PCM_STATE_RUNNING);
            return false;
        case SND_SEQ_EVENT_CLOCK:
            on_clock(midi_event.data.queue.unused[0]);
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
        case Command::PC_NEXT:
            program_change->select_next();
            return;
        case Command::PC_PREV:
            program_change->select_prev();
            return;
        case Command::NOOP:
            break;
    }
}

void Engine::exit(EngineExit _exit_code) {
    exit_code = _exit_code;
    SPDLOG_INFO("ENGIN exit[exit_code={}]", (int)exit_code);
    running_flag.clear();
    running_flag.notify_all();    
}

void Engine::on_clock(uint8_t quarter_note_fraction) {
    Pattern& pattern = session.get_pattern();
    PatternLoop& last_loop = pattern.get_loops().back();
    session.on_clock(quarter_note_fraction, loop_tracks.at(last_loop.track_number - 1)->get_position(last_loop.loop));
    if (quarter_note_fraction == 0) {
        program_change->on_quarter_note_clock();
    }
}

void Engine::on_controller(uint8_t channel, unsigned int param, signed int value, bool running) {
    if (channel != SOURCE_MIDI_CHANNEL || !running) {
        return;
    }
    ssize_t track_number;
    if ((track_number = loop_encoders.handle(channel, param, value)) > 0) {
        update_saturation(track_number);
        return;
    }
    if (fade_encoder.handle(channel, param, value)) {
        program_change->on_fader(fade_encoder.get_percentage());
        return;
    }
}

void Engine::update_saturation(ssize_t track_number) {
    for (auto& track : loop_tracks) {
        if (track->get_track_number() == track_number) {
            MidiEncoder& encoder = loop_encoders.get_encoder(track_number);
            if (encoder.is_grabbed()) {
                track->set_saturation(encoder.get_percentage());
                display.get_main_screen().set_loop_state(track_number - 1, true, track->get_saturation());
                display.get_main_screen().set_loop_grabbed(track_number - 1, true);
                display.tick(true);
            }
            return;
        }
    }
}

snd_pcm_uframes_t Engine::compute_latency(uint8_t mul, uint8_t clock_interval) {
    last_computed_latency = ((snd_pcm_uframes_t)mul * (snd_pcm_uframes_t)clock_interval * session.get_sample_rate()) / 1000;
    return last_computed_latency;
}

bool Engine::change_program(bool running, BankPattern bank_pattern) {
    if (!running && session.change_program(bank_pattern)) {
        program_change->on_program_changed(running);
        SPDLOG_INFO("ENGIN program set={}", session.get_pattern().get_bank_pattern().get_pattern());    
        return true;
    }
    return false;
}

std::vector<std::reference_wrapper<ClipPlayer>> Engine::add_loop_clips(Pattern& pattern) {
    std::vector<std::reference_wrapper<ClipPlayer>> result = std::vector<std::reference_wrapper<ClipPlayer>>();
    for (PatternLoop& pattern_loop : pattern.get_loops()) {
        size_t track_index = pattern_loop.track_number - 1;
        if (track_index >= ENGINE_LOOP_TRACKS) {
            throw OstrostrojException(fmt::format("Invalid track index! [track_index={},loop={}]", track_index, pattern_loop.loop.filename().string()));
        }
        auto& track = loop_tracks.at(track_index);
        ClipPlayer& clip_player = track->add_clip(session.get_clip(pattern_loop.loop));
        result.push_back(clip_player);
        track->set_saturation(0.0);
        loop_encoders.get_encoder(track->get_track_number()).set_percentage(0.0);
        SPDLOG_DEBUG("ENGIN clip added [track_index={},loop={}]", track_index, pattern_loop.loop.c_str());
    }
    return result;
}

void Engine::remove_clip_player(Clip& clip) {
    for (auto& track : loop_tracks) {
        track->remove_clip_player(clip);
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

    return result;
}

void Engine::init_display() {
    size_t mem_size_bytes = session.get_mem_size_bytes();
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

Engine::Engine(Project& _project, Display& _display, std::atomic_flag& _running_flag):
    alsa_midi(),
    session(_project, _display, ENGINE_LOOP_TRACKS),
    alsa_pcm(session.get_sample_rate()),
    display(_display),
    running_flag(_running_flag),
    loop_encoders(SOURCE_MIDI_CHANNEL, L1_PARAM),
    fade_encoder(MidiEncoder(SOURCE_MIDI_CHANNEL, 118, 0, 127)),
    command_controller(SOURCE_MIDI_CHANNEL),
    midi_flag(ATOMIC_FLAG_INIT),
    stop(false),
    loop_tracks {
        std::make_unique<Track>(1, 1, alsa_pcm.get_period_size(), alsa_pcm.get_periods()),
        std::make_unique<Track>(2, 1, alsa_pcm.get_period_size(), alsa_pcm.get_periods()),
        std::make_unique<Track>(3, 1, alsa_pcm.get_period_size(), alsa_pcm.get_periods()),
        std::make_unique<Track>(4, 1, alsa_pcm.get_period_size(), alsa_pcm.get_periods()),
        std::make_unique<Track>(5, 1, alsa_pcm.get_period_size(), alsa_pcm.get_periods()),
        std::make_unique<Track>(6, 2, alsa_pcm.get_period_size(), alsa_pcm.get_periods()),
    },
    track_fifos(init_track_fifos()),
    worker_sleep_time(std::chrono::microseconds(alsa_pcm.get_period_time()).count() / 2),
    workers(create_workers()),
    exit_code(EngineExit::ENGINE_EXIT_NOOP),
    last_computed_latency(0) {
    init_display();
    alsa_midi.start(std::bind(&Engine::midi_callback, this));
    try {
        alsa_pcm.start(
            std::bind(&Engine::pcm_event_callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
            std::bind(&Engine::pcm_callback, this, std::placeholders::_1));
    } catch (std::exception const &ex) {
        SPDLOG_ERROR(ex.what());
        throw;
    }
    program_change = std::make_unique<ProgramChange>(*this);
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
}

int Engine::get_loop_track_count() const {
    return loop_tracks.size();    
}

EngineExit Engine::get_exit_code() const {
    return exit_code;
}