#include "common.hpp"
#include "engine.hpp"

Engine::Engine(Project& project, AlsaMidi& alsa_midi, AlsaPcm& _alsa_pcm) {    
}

Engine::~Engine() {    
}

int Engine::get_loop_track_count() const {
    return 0;    
}

void Engine::pcm_callback() {    
}

void Engine::midi_callback() {    
}

/*#define SPDLOG_ACTIVE_LEVEL 2

#include "common.hpp"
#include "engine.hpp"
#include "profiler.hpp"

#define MAX_TASK_COUNT 64

Engine::Engine(Project& _project, AlsaMidi& alsa_midi, AlsaPcm& _alsa_pcm):
    project(_project),
    alsa_pcm(_alsa_pcm),
    midi_fifo(alsa_midi.get_fifo()),
    next_flag(ATOMIC_FLAG_INIT),
    loop_tracks {
        std::make_unique<Track>(1, _alsa_pcm.get_channel_fifo(0)),
        std::make_unique<Track>(2, _alsa_pcm.get_channel_fifo(1)),
        std::make_unique<Track>(3, _alsa_pcm.get_channel_fifo(2)),
        std::make_unique<Track>(4, _alsa_pcm.get_channel_fifo(3)),
        std::make_unique<Track>(5, _alsa_pcm.get_channel_fifo(4), _alsa_pcm.get_channel_fifo(5)),
        std::make_unique<Track>(6, _alsa_pcm.get_channel_fifo(6), _alsa_pcm.get_channel_fifo(7)),
    },
    one_shots_track(Track(7, _alsa_pcm.get_channel_fifo(8), _alsa_pcm.get_channel_fifo(9))),
    tasks(TrackTaskFifo(16)),
    interrupted(false),
    midi_processed(false),
    program_number(0) {
    static_assert(std::atomic_bool::is_always_lock_free);
    set_program(0);
    midi_start();
    create_threads();
}

Engine::~Engine() {   
    interrupted.store(true);
}

void Engine::create_threads() {
    for (unsigned int i = 0; i < std::thread::hardware_concurrency(); i++) {
        threads.emplace_back(std::bind(&Engine::run, this));
    }
    SPDLOG_INFO("{} worker threads created.", threads.size());
}

void Engine::run() {
    SPDLOG_DEBUG("Engine started.");
    try {
        while (!interrupted) {
#ifdef PROFILING            
            Profiler::get().engine_run_count++;
#endif            
            process_midi();
            run_tasks();
            next_flag.test_and_set();
            next_flag.wait(true);
        }
        SPDLOG_DEBUG("Engine interrupted.");
    } catch(std::exception const& e) {
        SPDLOG_ERROR(e.what());
    }
    SPDLOG_DEBUG("Engine done.");
}

void Engine::create_tasks() {
    for (std::unique_ptr<Track>& track : loop_tracks) {
        create_track_task(*track);
    }
    create_track_task(one_shots_track);
    next_flag.clear();
    next_flag.notify_all(); 
}

void Engine::create_track_task(Track& track) {
    if (tasks.push(std::bind(&Track::fill_output, &track))) {
#ifdef PROFILING
        Profiler::get().engine_tasks_count++;
#endif
    } else {
        SPDLOG_WARN("Tasks overflow!");
    }
}

void Engine::run_tasks() {     
    std::function<void(void)> task;
#ifdef PROFILING
    Profiler& profiler = Profiler::get();
#endif
    while (tasks.pop(task)) {
#ifdef PROFILING
        profiler.engine_tasks_count--;
        profiler.engine_tasks_in_progress++;
#endif
        task();
#ifdef PROFILING
        profiler.engine_tasks_in_progress--;
#endif
    }
#ifdef PROFILING
    profiler.tasks_done();
#endif
}

void Engine::process_midi() {
    if (!midi_processed) {
        std::lock_guard lk(midi_processing_mutex);
        if (!midi_processed) {
#ifdef PROFILING
            Profiler::get().next_engine_phase();
#endif
            snd_seq_event_t midi_message;
            while (midi_fifo.pop(midi_message)) {
                SPDLOG_INFO("Processing MIDI message. [0x{:x}]", static_cast<int>(midi_message.type));
                switch (midi_message.type) {
                    case SND_SEQ_EVENT_START:
                        midi_start();
                        break;
                    case SND_SEQ_EVENT_STOP:
                        midi_stop();
                        break;
                    case SND_SEQ_EVENT_CONTINUE:
                        midi_continue();
                        break;
                    case SND_SEQ_EVENT_PGMCHANGE:
                        set_program(midi_message.data.control.value);
                        break;
                    default:
                        break;
                }
            }
            midi_processed = true;
            create_tasks();
        }
    }
}

void Engine::midi_start() {
    // alsa_pcm.play_start();
    for (std::unique_ptr<Track>& track : loop_tracks) {
        track->start();
    }
    one_shots_track.start();
}

void Engine::midi_stop() {
    alsa_pcm.play_stop();
    for (std::unique_ptr<Track>& track : loop_tracks) {
        track->stop();
    }
    one_shots_track.stop();
}

void Engine::midi_continue() {
    alsa_pcm.play_continue();
    for (std::unique_ptr<Track>& track : loop_tracks) {
        track->start();
    }
    one_shots_track.start();
}

void Engine::play_one_shot(uint8_t _note) {
    SPDLOG_DEBUG("play_one_shot({})", _note);
}

void Engine::set_program(int _program_number) {
    program_number = _program_number;
    for (std::unique_ptr<Track>& track : loop_tracks) {
        track->reset_node();
    }
    Program& active_program = project.get_program(program_number);
    for (LoopClip& loop_clip : active_program.get_loops()) {
        loop_tracks.at(loop_clip.get_track())->set_node(std::make_unique<ClipNode>(loop_clip, true));
    }
    SPDLOG_INFO("Program set. [{}]", program_number.load());
}

int Engine::get_loop_track_count() const {
    return loop_tracks.size();
}

void Engine::pcm_callback() {
    tasks.push(std::bind(&Engine::create_tasks, this));
    next_flag.clear();
    next_flag.notify_one();
}

void Engine::midi_callback() {
    static std::atomic_bool started = false;
    if (!started.exchange(true)) {
        SPDLOG_WARN("midi_callback play_start");
        alsa_pcm.play_start();
    }
    midi_processed = false;
    next_flag.clear();
    next_flag.notify_one();
}
*/