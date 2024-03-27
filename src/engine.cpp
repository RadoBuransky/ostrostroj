#include <spdlog/spdlog.h>
#include "engine.hpp"

#define MAX_TASK_COUNT 16

void EngineTask::run(EngineTaskFifo &tasks) {
    // TODO: 
}

Engine::Engine(const Project& project, SoundCard& soundCard):
    project(project),
    soundCard(soundCard),
    threads(create_threads()),
    tasks(EngineTaskFifo(16)),
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
        process_audio();
    }
}

void Engine::process_audio() {     
    EngineTask task;   
    while (tasks.pop(task)) {
        task.run(tasks);
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
                // project_status.set_program(3);
            }
            // TODO: Enqueue engine tasks
            // project_status.get_loops();
            // TODO: Map LoopSample.get_track() to audio output port.
            midi_processed = true;
        }
    }
}

void Engine::next() {
    midi_processed = false;
    next_flag.clear();
    next_flag.notify_all();
}