#include <spdlog/spdlog.h>
#include "engine.hpp"
#include "common.hpp"
#include "profiler.hpp"

#define MAX_TASK_COUNT 64

bool Track::push_next_frame() {
    if (next_frame.empty()) {
        return true;
    }
    for (unsigned int channel = 0; channel < channels.size(); channel++) {
        if (!channels[channel].get().push(std::move(next_frame.at(channel)))) {
            spdlog::warn(std::format("Next frame overflow! [track={}, {} ch]", track_number, channel));
            if (channel != 0) {
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

void Track::get_clip_nodes(Node& node, std::vector<std::reference_wrapper<ClipNode>>& result) {
    ClipNode* clip_node = dynamic_cast<ClipNode*>(&node);
    if (clip_node) {
        result.push_back(std::ref(*clip_node));
        return;
    }
    ChildNode* child_node = dynamic_cast<ChildNode*>(&node);
    if (child_node) {
        get_clip_nodes(child_node->get_parent(), result);
    }
    MixingNode* mixing_node = dynamic_cast<MixingNode*>(&node);
    if (mixing_node) {
        for (std::reference_wrapper<Node> parent : mixing_node->get_parents()) {
            get_clip_nodes(parent, result);
        }
    }
}

void Track::preload_clips() {
    std::vector<std::reference_wrapper<ClipNode>>::iterator it = clips_to_load.begin();
    while (it != clips_to_load.end()) {
        if (!(*it).get().get_clip().load_next()) {
            spdlog::info(std::format("Clip preloaded. [{}]", it->get().get_clip().get_path().c_str()));
            it = clips_to_load.erase(it);
        } else {
            it++;
        }
    }
}

Track::Track(int _track_number, AudioFifo& channel):
    Track(_track_number, std::vector<std::reference_wrapper<AudioFifo>>({std::ref(channel)})) {
}

Track::Track(int _track_number, AudioFifo& left_channel, AudioFifo& right_channel):
    Track(_track_number, std::vector<std::reference_wrapper<AudioFifo>>({std::ref(left_channel), std::ref(right_channel)})) {
}

Track::Track(int _track_number, std::vector<std::reference_wrapper<AudioFifo>> _channels):
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
    clips_to_load.clear();
    get_clip_nodes(*node, clips_to_load);
    dynamic_node.set_parent(std::move(node));
}

void Track::reset_node() {
    clips_to_load.clear();
    dynamic_node.reset_parent();
}

void Track::fill_output() {
    for (std::reference_wrapper<AudioFifo>& channel : channels) {
        AudioFifo& fifo = channel.get();
        while (fifo.push(1.0)) {            
        }
    }

    /*
    if (!push_next_frame()) {
        return;
    }
    const int channels_size = channels.size();
    int channel = channels_size - 1;
    bool overflow = false;
    float sample;
    Profiler& profiler = Profiler::get();
    while (!overflow && track_node.pop(sample)) {
        channel = (channel + 1) % channels_size;
        overflow = !channels[channel].get().push(std::move(sample));
        profiler.engine_samples_pushed++;
    }
    if (overflow) {
        if (channel != 0) {
            spdlog::warn(std::format("FIFO not channel-aligned! [track={}, {} ch]", track_number, channel));
            return;
        }
        pop_next_frame(sample);
    } else {
        spdlog::warn("Track underrun!");
    }
    preload_clips();*/
}

Engine::Engine(Project& _project, SoundCard& _soundCard):
    project(_project),
    soundCard(_soundCard),
    loop_tracks {
        std::make_unique<Track>(1, _soundCard.get_audio_output_fifo(0)),
        std::make_unique<Track>(2, _soundCard.get_audio_output_fifo(1)),
        std::make_unique<Track>(3, _soundCard.get_audio_output_fifo(2)),
        std::make_unique<Track>(4, _soundCard.get_audio_output_fifo(3)),
        std::make_unique<Track>(5, _soundCard.get_audio_output_fifo(4), _soundCard.get_audio_output_fifo(5)),
        std::make_unique<Track>(6, _soundCard.get_audio_output_fifo(6), _soundCard.get_audio_output_fifo(7)),
    },
    one_shots_track(Track(7, _soundCard.get_audio_output_fifo(8), _soundCard.get_audio_output_fifo(9))),
    tasks(TrackTaskFifo(16)),
    interrupted(false),
    next_flag(ATOMIC_FLAG_INIT),
    midi_processed(false),
    program_number(0) {
    static_assert(std::atomic_bool::is_always_lock_free);
    create_threads();
}

Engine::~Engine() {   
    interrupted.store(true);
    next_flag.clear();
    next_flag.notify_all();    
}

void Engine::create_threads() {
    for (unsigned int i = 0; i < std::thread::hardware_concurrency(); i++) {
        threads.emplace_back(std::bind(&Engine::run, this));
    }
    spdlog::info(std::format("{} worker threads created.", threads.size()));
}

void Engine::run() {
    spdlog::debug("Engine started.");
    try {
        while (!interrupted) {
            next_flag.test_and_set();
            next_flag.wait(true);
            Profiler::get().engine_run_count++;
            process_midi();
            run_tasks();
            long last_done_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
            if (last_done_timestamp > next_timestamp) {
                last_duration_ns = last_done_timestamp - next_timestamp;
            }
        }
        spdlog::debug("Engine interrupted.");
    } catch(std::exception const& e) {
        spdlog::error(e.what());
    }
    spdlog::debug("Engine done.");
}

void Engine::create_tasks() {
    // TODO: Can we use atomic_int for this?
    for (std::unique_ptr<Track>& track : loop_tracks) {
        create_track_task(*track);
    }
    create_track_task(one_shots_track);
}

void Engine::create_track_task(Track& track) {
    if (tasks.push(std::bind(&Track::fill_output, &track))) {
        Profiler::get().engine_tasks_count++;
    } else {
        spdlog::warn("Tasks overflow!");
    }
}

void Engine::run_tasks() {     
    std::function<void(void)> task;
    Profiler& profiler = Profiler::get();
    while (tasks.pop(task)) {
        profiler.engine_tasks_count--;
        profiler.engine_tasks_in_progress++;
        task();
        profiler.engine_tasks_in_progress--;
    }
}

void Engine::process_midi() {
    if (!midi_processed) {
        std::lock_guard lk(midi_processing_mutex);
        if (!midi_processed) {
            Profiler& profiler = Profiler::get();
            profiler.engine_phase_count++;
            profiler.engine_phase_total_duration += last_duration_ns;
            if (last_duration_ns > profiler.engine_phase_max_duration) {
                profiler.engine_phase_max_duration = last_duration_ns.load();
            }
            profiler.periodic_log();
            libremidi::message midi_message;
            MidiFifo& midi_fifo = soundCard.get_midi_fifo();
            while (midi_fifo.pop(midi_message)) {
                // spdlog::trace(std::format("Processing MIDI message. [0x{:x}]", static_cast<int>(midi_message.get_message_type())));
                switch (midi_message.get_message_type()) {
                    case libremidi::message_type::START:
                        midi_start();
                        break;
                    case libremidi::message_type::STOP:
                        midi_stop();
                        break;
                    case libremidi::message_type::CONTINUE:
                        midi_continue();
                        break;
                    case libremidi::message_type::PROGRAM_CHANGE:
                        set_program(midi_message.bytes[0]); // TODO: Is this ok?
                        break;
                    default:
                        break;
                }
            }
            create_tasks();
            midi_processed = true;
            next_flag.notify_all();
        }
    }
}

void Engine::midi_start() {
    set_program(program_number);
    for (std::unique_ptr<Track>& track : loop_tracks) {
        track->start();
    }
    one_shots_track.start();
}

void Engine::midi_stop() {
    for (std::unique_ptr<Track>& track : loop_tracks) {
        track->stop();
    }
    one_shots_track.stop();
    Profiler::get().log();
}

void Engine::midi_continue() {
    for (std::unique_ptr<Track>& track : loop_tracks) {
        track->start();
    }
    one_shots_track.start();
}

void Engine::play_one_shot(uint8_t _note) {
    // TODO: Unload sample from memory once done
    spdlog::debug(std::format("play_one_shot({})", _note));
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
    spdlog::info(std::format("Program set. [{}]", program_number.load()));
}

void Engine::next() {
    next_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    Profiler::get().engine_tasks_late += Profiler::get().engine_tasks_in_progress;
    midi_processed = false;
    next_flag.clear();
    next_flag.notify_one();
}

int Engine::get_loop_track_count() const {
    return loop_tracks.size();
}