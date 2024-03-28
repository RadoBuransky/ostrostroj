#include <spdlog/spdlog.h>
#include "engine.hpp"

#define MAX_TASK_COUNT 16

Engine::Engine(const Project& project, SoundCard& soundCard):
    project(project),
    soundCard(soundCard),
    threads(create_threads()),
    tasks(EngineTaskFifo(16)),
    interrupted(false),
    next_flag(ATOMIC_FLAG_INIT) {
    set_active_program(0);
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
        create_tasks();
        run_tasks();
    }
}

void Engine::process_midi() {
    if (!midi_processed) {
        std::lock_guard lk(midi_processing_mutex);
        if (!midi_processed) {
            libremidi::message midi_message;
            MidiFifo& midi_fifo = soundCard.get_midi_fifo();
            while (midi_fifo.pop(midi_message)) {
                // TODO: Process MIDI message (change internal state)
            }
            // TODO: Enqueue engine tasks
            // project_status.get_loops();
            // TODO: Map LoopSample.get_track() to audio output port.
            midi_processed = true;
        }
    }
}

// TODO:
// - tasks are state holders
// - one task per audio output fifo
// - tasks CAN'T create other tasks 
// - midi processing creates tasks with current state (mute, unmute, params):
//       - play loop
//       - play one shot
// - run tasks until:
//      - audio output fifos are full or
//      - 
void Engine::create_tasks() {
    tasks.push(std::bind(&Engine::play_one_shot, this, 13));
}

void Engine::run_tasks() {     
    EngineTask task;

    while (tasks.pop(task)) {
        task();
    }
}

void Engine::play_one_shot(uint8_t note) {
    SampleReader sampleReader = project.get_programs().at(0).get_one_shots().at(note).createReader();
}

void Engine::set_active_program(int program_number) {
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