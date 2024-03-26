#include <spdlog/spdlog.h>
#include "engine.hpp"

#define MAX_TASK_COUNT 16

void EngineTask::run(EngineTaskFifo &queue) {
    // TODO: 
}

Engine::Engine():
    threads(create_threads()),
    queue(EngineTaskFifo(16)),
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
    EngineTask task;
    while (!interrupted) {
        next_flag.test_and_set();
        next_flag.wait(true);

        if (!midi_processed) {
            std::lock_guard lk(midi_processing_mutex);
            if (!midi_processed) {
                libremidi::message midi_message;
                if (midi_fifo->pop(midi_message)) {
                    do {
                        // TODO: Process MIDI message (change internal state)
                    } while(midi_fifo->pop(midi_message));
                }
                midi_processed = true;
            }
        }

        // TODO: 2. How to decide 

        while (queue.pop(task)) {
            task.run(queue);
        }
    }
}

MidiFifo& Engine::get_midi_fifo() {
    return *midi_fifo.get();
}

void Engine::next() {
    midi_processed = false;
    next_flag.clear();
    next_flag.notify_all();
}