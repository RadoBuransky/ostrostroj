#include "common.hpp"
#include "track.hpp"

void Track::run() {
    InterleavedFifo& fifo_ref = *fifo.get();
    float in_sample;
    PcmSample_s24_3le out_sample;
    bool out_sample_pushed = true;
    SPDLOG_DEBUG("Track {} started. [channels={}]", track_number, channels);
    try {
        while (!stop) {            
            std::unique_lock<std::mutex> lock(m);
            out_sample_pushed = false;
            do {
                if (track_node.pop(in_sample)) {
                    out_sample = in_sample;
                } else {
                    SPDLOG_DEBUG("Track {} underrun.", track_number);
                    out_sample_pushed = true;
                    break;
                }
            } while (fifo_ref.push(std::move(out_sample)));

            cv.wait_for(lock, period_time);

            if (!out_sample_pushed) {
                if (!fifo_ref.push(std::move(out_sample))) {
                    continue;
                } else {
                    out_sample_pushed = true;
                }
            }
        }
        SPDLOG_DEBUG("Track {} stopped.", track_number);
    } catch(std::exception const& e) {
        SPDLOG_ERROR("Track {} failed. {}", track_number, e.what());
    }
}

Track::Track(int _track_number, int _channels, std::chrono::milliseconds _period_time, snd_pcm_uframes_t _period_size):
    track_number(_track_number),
    channels(_channels),
    period_time(_period_time),
    fifo(std::make_unique<InterleavedFifo>(_period_size * 2)),
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

int Track::get_channels() const {
    return channels;
}

void Track::reset_node() {
    {
        std::lock_guard<std::mutex> guard(m);
        dynamic_node.reset_parent();
    }
    cv.notify_all();
}

void Track::set_node(std::unique_ptr<Node>&& node) {
    {
        std::lock_guard<std::mutex> guard(m);
        dynamic_node.set_parent(std::move(node));
    }
    cv.notify_all();
}