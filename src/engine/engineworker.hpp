#pragma once

#include "track.hpp"

class EngineWorker {
    private:
        int worker_index;
        useconds_t sleep_time;
        std::atomic_bool stop;
        std::mutex mutex;
        std::unique_lock<std::mutex> tracks_lock;
        std::vector<std::reference_wrapper<Track>> tracks;
        std::thread thread;
        void run();
        void run_tracks();
        std::string tracks_mkstring();
    public:
        EngineWorker(std::vector<std::reference_wrapper<Track>> _tracks, int _worker_index, useconds_t _sleep_time);
        virtual ~EngineWorker();
        void lock_tracks();
        void unlock_tracks();
};