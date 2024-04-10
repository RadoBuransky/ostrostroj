#include <spdlog/spdlog.h>
#include "engine.hpp"
#include "common.hpp"

#define MAX_TASK_COUNT 16

bool Track::push_next_frame() {
    if (next_frame.empty()) {
        return true;
    }
    for (unsigned int channel = 0; channel < next_frame.size(); channel++) {
        AudioFifo& channel_fifo = channels.at(channel);
        if (!channel_fifo.push(std::move(next_frame.at(channel)))) {
            return false;
        }
    }
    next_frame.clear();
    return true;
}

void Track::pop_next_frame(float sample) {
    for (unsigned int channel = 0; channel < channels.size(); channel++) {
        next_frame.push_back(sample);
        if (!track_node.pop(sample)) {
            spdlog::warn("Next frame underrun!");
            next_frame.clear();
            return;
        }
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
        if (!(*it).get().load_next()) {
            it = clips_to_load.erase(it);
        } else {
            it++;
        }
    }
}

Track::Track(AudioFifo& channel):
    Track(std::vector<std::reference_wrapper<AudioFifo>>({std::ref(channel)})) {
}

Track::Track(AudioFifo& left_channel, AudioFifo& right_channel):
    Track(std::vector<std::reference_wrapper<AudioFifo>>({std::ref(left_channel), std::ref(right_channel)})) {
}

Track::Track(std::vector<std::reference_wrapper<AudioFifo>> _channels):
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
    spdlog::debug(std::format("{} (this=0x{:x}, node=0x{:x})",
    __FUNCTION__, reinterpret_cast<intptr_t>(this), reinterpret_cast<intptr_t>(node.get())));
    dynamic_node.set_parent(std::move(node));
}

void Track::reset_node() {
    clips_to_load.clear();
    dynamic_node.reset_parent();
}

void Track::fill_output() {
    float sample;
    bool overflow = false;
    int channel = channels.size() - 1;
    if (!push_next_frame()) {
        return;
    }
    while (!overflow && track_node.pop(sample)) {
        channel = (channel + 1) % channels.size();
        AudioFifo& channel_fifo = channels.at(channel);
        overflow = channel_fifo.push(std::move(sample));
    }
    if (overflow) {
        if (channel != 0) {
            spdlog::warn("FIFO not channel-aligned!");
            return;
        }
        pop_next_frame(sample);
    }
    preload_clips();
}

Engine::Engine(Project& _project, SoundCard& _soundCard):
    project(_project),
    soundCard(_soundCard),
    loop_tracks {
        std::make_unique<Track>(_soundCard.get_audio_output_fifo(0)),
        std::make_unique<Track>(_soundCard.get_audio_output_fifo(1)),
        std::make_unique<Track>(_soundCard.get_audio_output_fifo(2)),
        std::make_unique<Track>(_soundCard.get_audio_output_fifo(3)),
        std::make_unique<Track>(_soundCard.get_audio_output_fifo(4), _soundCard.get_audio_output_fifo(5)),
        std::make_unique<Track>(_soundCard.get_audio_output_fifo(6), _soundCard.get_audio_output_fifo(7)),
    },
    one_shots_track(Track(_soundCard.get_audio_output_fifo(8), _soundCard.get_audio_output_fifo(9))),
    tasks(TrackTaskFifo(16)),
    interrupted(false),
    next_flag(ATOMIC_FLAG_INIT),
    midi_processed(false),
    program_number(0) {
    create_threads();
}

Engine::~Engine() {   
    spdlog::trace("~Engine()");
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
    bool i = interrupted.load();
    spdlog::info(std::format("Engine started. [{}]", i));
    while (!i) {
        next_flag.test_and_set();
        spdlog::trace("Engine waiting...");
        next_flag.wait(true);
        try {
            process_midi();
            run_tasks();   
        } catch(std::exception const& e) {
            spdlog::error(e.what());
        }
        i = interrupted.load();
    }
    spdlog::info(std::format("Engine done. [{}]", i));
}

void Engine::create_tasks() {
    for (auto& track : loop_tracks) {
        create_track_task(*track);
    }
    create_track_task(one_shots_track);
}

void Engine::create_track_task(Track& track) {
    tasks.push(std::bind(&Track::fill_output, &track));
}

void Engine::run_tasks() {     
    std::function<void(void)> task;
    while (tasks.pop(task)) {
        task();
    }
}

void Engine::process_midi() {
    if (!midi_processed) {
        std::lock_guard lk(midi_processing_mutex);
        if (!midi_processed) {
            libremidi::message midi_message;
            MidiFifo& midi_fifo = soundCard.get_midi_fifo();
            while (midi_fifo.pop(midi_message)) {
                spdlog::trace(std::format("Processing MIDI message. [0x{:x}]", static_cast<int>(midi_message.get_message_type())));
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
        }
    }
}

void Engine::midi_start() {
    set_program(program_number);
    for (std::unique_ptr<Track>& track : loop_tracks) {
        track->start();
    }
    spdlog::info("Started.");
}

void Engine::midi_stop() {
    for (std::unique_ptr<Track>& track : loop_tracks) {
        track->stop();
    }
    spdlog::info("Stopped.");
}

void Engine::midi_continue() {
    for (std::unique_ptr<Track>& track : loop_tracks) {
        track->start();
    }
    spdlog::info("Continued.");
}

void Engine::play_one_shot(uint8_t _note) {
    // TODO: Unload sample from memory once done
}

void Engine::set_program(int _program_number) {
    program_number = _program_number;
    for (std::unique_ptr<Track>& track : loop_tracks) {
        track->reset_node();
        spdlog::debug("track reset");
    }
    Program& active_program = project.get_program(program_number);
    for (LoopClip& loop_clip : active_program.get_loops()) {
        loop_tracks.at(loop_clip.get_track())->set_node(std::make_unique<ClipNode>(loop_clip, true));
        spdlog::debug("node set");
    }
    spdlog::info(std::format("Program set. [{}]", program_number));
}

void Engine::next() {
    midi_processed = false;
    next_flag.clear();
    next_flag.notify_all();
#ifndef NDEBUG
    // spdlog::trace("Engine::next()");
#endif
}

int Engine::get_loop_track_count() const {
    return loop_tracks.size();
}