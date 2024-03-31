#include <spdlog/spdlog.h>
#include "engine.hpp"

#define MAX_TASK_COUNT 16

Track::Track(AudioFifo& channel):
    Track(std::vector<std::reference_wrapper<AudioFifo>>({std::ref(channel)})) {
}

Track::Track(AudioFifo& left_channel, AudioFifo& right_channel):
    Track(std::vector<std::reference_wrapper<AudioFifo>>({std::ref(left_channel), std::ref(right_channel)})) {
}

Track::Track(std::vector<std::reference_wrapper<AudioFifo>> _channels):
    channels(_channels),
    dynamic_node(DynamicNode()),
    mute_node(MuteNode(dynamic_node)),
    transport_node(TransportNode(mute_node)) {
}

void Track::set_mute(bool mute) {
    mute_node.set_mute(mute);
}

void Track::start() {
    transport_node.start();
}

void Track::stop() {
    transport_node.stop();
}

void Track::set_node(std::unique_ptr<Node> node) {
    dynamic_node.set_parent(std::move(node));
}

void Track::reset_node() {
    dynamic_node.reset_parent();
}

void Track::fill_output() {
    float sample;
    int channel = 0;
    while (transport_node.pop(sample)) {
        AudioFifo& channel_fifo = channels.at(channel);
        channel_fifo.push(std::move(sample));
        channel = (channel + 1) % channels.size();
    }
}

Engine::Engine(Project& project, SoundCard& soundCard):
    project(project),
    soundCard(soundCard),
    loop_tracks {
        std::make_unique<Track>(soundCard.get_audio_output_fifo(0)),
        std::make_unique<Track>(soundCard.get_audio_output_fifo(1)),
        std::make_unique<Track>(soundCard.get_audio_output_fifo(2)),
        std::make_unique<Track>(soundCard.get_audio_output_fifo(3)),
        std::make_unique<Track>(soundCard.get_audio_output_fifo(4), soundCard.get_audio_output_fifo(5)),
        std::make_unique<Track>(soundCard.get_audio_output_fifo(6), soundCard.get_audio_output_fifo(7)),
    },
    one_shots_track(Track(soundCard.get_audio_output_fifo(8), soundCard.get_audio_output_fifo(9))),
    threads(create_threads()),
    tasks(TrackTaskFifo(16)),
    interrupted(false),
    next_flag(ATOMIC_FLAG_INIT) {
    set_program(0);
}

Engine::~Engine() {   
    interrupted = true;
    next_flag.clear();
    next_flag.notify_all();    
}

std::vector<std::thread> Engine::create_threads() {
    std::vector<std::thread> result(0);
    for (auto i = 0; i < std::thread::hardware_concurrency(); i++) {
        result.push_back(std::thread(&Engine::run, this));
    }
    return result;
}

void Engine::run() {
    while (!interrupted) {
        next_flag.test_and_set();
        next_flag.wait(true);
        process_midi();
        run_tasks();
    }
}

void Engine::create_tasks() {
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
                        set_program(midi_message.bytes[0]);
                        break;
                }
            }
            create_tasks();
            midi_processed = true;
        }
    }
}

void Engine::midi_start() {
    // TODO: Reset position (start from beginning)
    for (std::unique_ptr<Track>& track : loop_tracks) {
        track->start();
    }
}

void Engine::midi_stop() {
    for (std::unique_ptr<Track>& track : loop_tracks) {
        track->stop();
    }
}

void Engine::midi_continue() {
    for (std::unique_ptr<Track>& track : loop_tracks) {
        track->start();
    }
}

void Engine::play_one_shot(uint8_t note) {
    // TODO: Unload sample (not samplereader!) from memory once done
}

void Engine::set_program(int program_number) {
    for (std::unique_ptr<Track>& track : loop_tracks) {
        track->reset_node();
    }

    Program& active_program = project.get_program(program_number);
    for (LoopSample& loop_sample : active_program.get_loops()) {
        auto sample_node = std::make_unique<SampleNode>(loop_sample, true);
        loop_tracks[loop_sample.get_track()]->set_node(std::move(sample_node));
    }
}

void Engine::next() {
    midi_processed = false;
    next_flag.clear();
    next_flag.notify_all();
}