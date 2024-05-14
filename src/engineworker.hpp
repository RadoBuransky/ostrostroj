#pragma once

#include "track.hpp"

class EngineWorker {
    private:
        int worker_index;
        useconds_t sleep_time;
        std::atomic_bool stop;
        std::mutex mutex;
        std::condition_variable cv;
        std::vector<std::reference_wrapper<Track>> tracks;
        std::thread thread;
        void run();
        bool run_tracks();
    public:
        EngineWorker(int _worker_index, useconds_t _sleep_time);
        virtual ~EngineWorker();
        void assign_tracks(std::vector<std::reference_wrapper<Track>> _tracks);
        void release_tracks();
};