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
    interrupted(false) {
}

Engine::~Engine() {   
    interrupted.store(true);
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
    while (!interrupted.load()) {
        if (queue.pop(task)) {
            if (task.run()) {
                // TODO: Avoid busy loop if the task did nothing
                queue.push(std::move(task));
            }
        } else {
            // TODO: 1/2 buffer length + jitter
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}