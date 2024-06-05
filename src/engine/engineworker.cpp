#include "common.hpp"
#include "engineworker.hpp"

void EngineWorker::run() {
    try {
        SPDLOG_INFO("EW{}   started [sleep_time={}us]", worker_index, sleep_time);
        while (!stop) {
            run_tracks();
            usleep(sleep_time);
        }
        SPDLOG_INFO("EW{}   stopped", worker_index);
    } catch(std::exception const& e) {
        SPDLOG_ERROR("EW{}   failed. {}", worker_index, e.what());
    }
}

void EngineWorker::run_tracks() {
    std::unique_lock lock(mutex);
    for (std::reference_wrapper<Track> track: tracks) {
        track.get().run();
    }
}

EngineWorker::EngineWorker(std::vector<std::reference_wrapper<Track>> _tracks, int _worker_index, useconds_t _sleep_time):
    worker_index(_worker_index),
    sleep_time(_sleep_time),
    stop(false),
    mutex(),
    tracks_lock(mutex, std::defer_lock),
    tracks(_tracks),
    thread(std::bind(&EngineWorker::run, this)) {
    if (tracks.empty()) {
        throw OstrostrojException(fmt::format("EW{}   no tracks!", _worker_index));
    }
    pthread_setname_np(thread.native_handle(), fmt::format("worker{}", _worker_index).c_str());
}

EngineWorker::~EngineWorker() {
    stop = true;
    tracks_lock.release();
    thread.join();
}

void EngineWorker::lock_tracks() {
    tracks_lock.lock();
}

void EngineWorker::release_tracks() {
    tracks_lock.release();
}