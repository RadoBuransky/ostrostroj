#include <spdlog/spdlog.h>
#include "engine.hpp"

#define MAX_TASK_COUNT 16

Track::Track(AudioFifo& channel):
    channels({std::ref(channel)}) {
}

Track::Track(AudioFifo& left_channel, AudioFifo& right_channel):
    channels({std::ref(left_channel), std::ref(right_channel)}) {
}

void Track::set_node(std::unique_ptr<Node>& _node) {
    node = std::move(_node);
}

void Track::fill_output() {
    float sample;
    int channel = 0;
    while (node->pop(channel, sample)) {
        AudioFifo& channel_fifo = channels.at(channel);
        channel_fifo.push(std::move(sample));
        channel = (channel + 1) % channels.size();
    }
}

Engine::Engine(const Project& project, SoundCard& soundCard):
    project(project),
    soundCard(soundCard),
    active_program(project.get_program(0)),
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
                        break;
                    case libremidi::message_type::CONTINUE:
                        break;
                    case libremidi::message_type::PROGRAM_CHANGE:
                        set_active_program(midi_message.bytes[0]);
                        break;
                }
            }
            create_tasks();
            midi_processed = true;
        }
    }
}

void Engine::midi_start() {
    // Create nodes
}

void Engine::play_one_shot(uint8_t note) {
    // SampleReader sampleReader = project.get_programs().at(0).get_one_shots().at(note).createReader();
}

void Engine::set_active_program(int program_number) {
    active_program = project.get_program(program_number);
    // TODO:
    // 1. Find program for this program_number
    // 2. if different than the current, unload the current and load the new one
    // 3. create tasks for new loops
}

void Engine::next() {
    midi_processed = false;
    next_flag.clear();
    next_flag.notify_all();
}