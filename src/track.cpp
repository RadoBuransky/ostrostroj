#define SPDLOG_ACTIVE_LEVEL 2

#include "common.hpp"
#include "track.hpp"

void Track::run() {
    InterleavedFifo& fifo_ref = *fifo.get();
    float in_sample;
    PcmSample_s24_3le out_sample;
    SPDLOG_INFO("Track {} started. [channels={},period_time={}]", track_number, channels, std::chrono::microseconds(period_time).count());
    try {
        std::unique_lock<std::mutex> lock(m);
        while (!stop) {            
            do {
                if (track_node.pop(in_sample)) {
                    out_sample = in_sample;
                } else {
                    if (no_xrun) {
                        // TODO: Be careful because we're desyncing tracks here
                        out_sample.silence();
                    } else {
                        SPDLOG_DEBUG("Track {} is waiting empty...", track_number);
                        cv.wait(lock);
                        SPDLOG_DEBUG("Track {} activated", track_number);
                        continue;
                    }
                }
            } while (fifo_ref.push(std::move(out_sample)));
            do {
                if (cv.wait_for(lock, period_time) == std::cv_status::no_timeout) {
                    break;
                }
            } while (!fifo_ref.push(std::move(out_sample)));
        }
        SPDLOG_INFO("Track {} stopped.", track_number);
    } catch(std::exception const& e) {
        SPDLOG_ERROR("Track {} failed. {}", track_number, e.what());
    }
}

Track::Track(int _track_number, int _channels, std::chrono::milliseconds _period_time, snd_pcm_uframes_t _period_size, bool _no_xrun):
    track_number(_track_number),
    channels(_channels),
    period_time(_period_time),
    no_xrun(_no_xrun),
    // TODO: Increase this to ALSA PCM buffer size?
    fifo(std::make_unique<InterleavedFifo>(_period_size * 2 * _channels)),
    stop(false),
    dynamic_node(),
    track_node(dynamic_node),
    m(),
    worker_thread(std::bind(&Track::run, this)) {
    pthread_setname_np(worker_thread.native_handle(), fmt::format("track{}", track_number).c_str());
}

Track::~Track() {
    stop = true;
    worker_thread.join();
}

InterleavedFifo& Track::get_fifo() const {
    return *fifo.get();
}

int Track::get_track_number() const {
    return track_number;
}

int Track::get_channels() const {
    return channels;
}

void Track::reset_node() {
    bool reset;
    {
        std::lock_guard<std::mutex> guard(m);
        reset = dynamic_node.reset_parent();
    }
    if (reset) {
        cv.notify_all();
    }
}

void Track::set_node(std::unique_ptr<Node>&& node) {
    {
        std::lock_guard<std::mutex> guard(m);
        dynamic_node.set_parent(std::move(node));
    }
    cv.notify_all();
}