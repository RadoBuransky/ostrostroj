#include "common.hpp"

#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>

#include "engineworker.hpp"

void EngineWorker::run() {
    try {
        SPDLOG_INFO("EW{}   started [tracks={},sleep_time={}us]", worker_index, tracks_mkstring(), sleep_time);
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
    SPDLOG_TRACE("EW{}   run_tracks acquiring lock...", worker_index);
    std::unique_lock lock(mutex);
    SPDLOG_TRACE("EW{}   run_tracks lock acquired", worker_index);
    for (std::reference_wrapper<Track> track: tracks) {
        track.get().run();
    }
    SPDLOG_TRACE("EW{}   run_tracks done", worker_index);
}

std::string EngineWorker::tracks_mkstring() {
    std::string s = "";
    for (std::reference_wrapper<Track> track : tracks) {
        s.append(std::to_string(track.get().get_track_number()));
        s.append(" ");
    }
    return s;
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
    SPDLOG_TRACE("EW{}   stopping...", worker_index);
    stop = true;
    tracks_lock.release();
    thread.join();
}

void EngineWorker::lock_tracks() {
    tracks_lock.lock();
    SPDLOG_TRACE("EW{}   lock_tracks", worker_index);
}

void EngineWorker::unlock_tracks() {
    tracks_lock.unlock();
    SPDLOG_TRACE("EW{}   unlock_tracks", worker_index);
}

std::vector<std::reference_wrapper<Track>>& EngineWorker::get_tracks() {
    return tracks;
}