#define SPDLOG_ACTIVE_LEVEL 2

#include "common.hpp"
#include "engine.hpp"
#include "profiler.hpp"

#define MAX_TASK_COUNT 64

bool Track::push_next_frame() {
    if (next_frame.empty()) {
        return true;
    }
    for (unsigned int channel = 0; channel < channels.size(); channel++) {
        if (!channels[channel].get().push(std::move(next_frame.at(channel)))) {
            if (channel != 0) {
                SPDLOG_WARN("Next frame overflow! [track={}, {} ch]", track_number, channel);
                next_frame.clear();
            }
            return false;
        }
    }
    next_frame.clear();
    return true;
}

void Track::pop_next_frame(float sample) {    
    next_frame.push_back(sample);
    for (unsigned int channel = 1; channel < channels.size(); channel++) {
        if (!track_node.pop(sample)) {
            next_frame.clear();
            return;
        }
        next_frame.push_back(sample);
    }
}

Track::Track(int _track_number, PcmFifo& channel):
    Track(_track_number, std::vector<std::reference_wrapper<PcmFifo>>({std::ref(channel)})) {
}

Track::Track(int _track_number, PcmFifo& left_channel, PcmFifo& right_channel):
    Track(_track_number, std::vector<std::reference_wrapper<PcmFifo>>({std::ref(left_channel), std::ref(right_channel)})) {
}

Track::Track(int _track_number, std::vector<std::reference_wrapper<PcmFifo>> _channels):
    track_number(_track_number),
    channels(_channels),
    dynamic_node(DynamicNode()),
    track_node(TrackNode(dynamic_node)) {
}

void Track::set_mute(bool mute) {
    track_node.set_mute(mute);
}

void Track::start() {
    track_node.start();
}

void Track::stop() {
    track_node.stop();
}

void Track::set_node(std::unique_ptr<Node>&& node) {
    dynamic_node.set_parent(std::move(node));
}

void Track::reset_node() {
    dynamic_node.reset_parent();
}

void Track::fill_output() {
    if (!push_next_frame()) {
        return;
    }
    const int channels_size = channels.size();
    int channel = channels_size - 1;
    bool overflow = false;
    float sample;
#ifdef PROFILING
    Profiler& profiler = Profiler::get();
#endif
    while (!overflow && track_node.pop(sample)) {
        channel = (channel + 1) % channels_size;
        overflow = !channels[channel].get().push(std::move(sample));
#ifdef PROFILING
        profiler.engine_samples_pushed++;
#endif
    }
    SPDLOG_DEBUG("Track full. [{}]", track_number);
    if (overflow) {
        if (channel != 0) {
            // TODO: It is possible that alsapcm popped channel 0 but not popped channel 1 yet. Concurrency.
            SPDLOG_WARN("FIFO not channel-aligned! [track={}, {}/{} ch]", track_number, channel, channels_size);
            return;
        }
        pop_next_frame(sample);
    } else {
        SPDLOG_WARN("Track underrun!");
    }
}

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
    create_threads();
    set_program(0);
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
    alsa_pcm.play_start();
    midi_processed = false;
    next_flag.clear();
    next_flag.notify_one();
}