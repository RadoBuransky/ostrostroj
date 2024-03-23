#include <spdlog/spdlog.h>
#include "engine.hpp"

#define MAX_TASK_COUNT 16

AudioOutputTask::AudioOutputTask() {
}

AudioOutputTask::~AudioOutputTask() {    
}

bool AudioOutputTask::run() const {
    return true;
}

Engine::Engine():
    threads(create_threads()),
    queue(AudioOutputTaskFifo(16)),
    next_queue(AudioOutputTaskFifo(16)),
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
    AudioOutputTask task;
    while (!interrupted) {
        next_flag.test_and_set();
        next_flag.wait(true);
        while (queue.pop(task)) {
            if (task.run()) {
                next_queue.push(std::move(task));
            }
        }
    }
}

void Engine::next() {
    AudioOutputTask task;
    while (next_queue.pop(task)) {
        queue.push(std::move(task));
    }
    next_flag.clear();
    next_flag.notify_all();    
}