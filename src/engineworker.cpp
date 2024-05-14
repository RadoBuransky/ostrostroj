#include "common.hpp"
#include "engineworker.hpp"

void EngineWorker::run() {
    try {
        SPDLOG_INFO("EW{}   started [sleep_time={}us]", worker_index, sleep_time);
        while (!stop) {
            if (run_tracks()) {
                usleep(sleep_time);
            }
        }
        SPDLOG_INFO("EW{}   stopped", worker_index);
    } catch(std::exception const& e) {
        SPDLOG_ERROR("EW{}   failed. {}", worker_index, e.what());
    }
}

bool EngineWorker::run_tracks() {
    std::unique_lock lock(mutex);
    if (tracks.empty()) {
        SPDLOG_DEBUG("EW{}   waiting for tracks...", worker_index);
        cv.wait(lock, [&]{ return stop || !tracks.empty(); });
        SPDLOG_DEBUG("EW{}   waiting done. [tracks={}]", worker_index, tracks.size());
        return false;
    }
    for (std::reference_wrapper<Track> track: tracks) {
        track.get().run();
    }
    return true;
}

EngineWorker::EngineWorker(int _worker_index, useconds_t _sleep_time):
    worker_index(_worker_index),
    sleep_time(_sleep_time),
    stop(false),
    mutex(),
    cv(),
    tracks(),
    thread(std::bind(&EngineWorker::run, this)) {    
    pthread_setname_np(thread.native_handle(), fmt::format("worker{}", _worker_index).c_str());
}

EngineWorker::~EngineWorker() {
    stop = true;
    cv.notify_one();
    thread.join();
}

void EngineWorker::assign_tracks(std::vector<std::reference_wrapper<Track>> _tracks) {
    std::lock_guard lock(mutex);
    tracks = _tracks;
    cv.notify_one();
    SPDLOG_INFO("EW{}   tracks set. [size={}]", worker_index, tracks.size());
}

void EngineWorker::release_tracks() {
    std::lock_guard lock(mutex);
    tracks.clear();
}