#include "profiler.hpp"

Profiler::Profiler():
    jack_sample_rate(0),
    jack_callback_count(0),
    jack_callback_total_frames(0),
    jack_callback_total_audio_frames(0),
    jack_callback_total_duration(0),
    jack_callback_fifo_underrun(0),
    engine_samples_pushed(0),
    next_log(std::chrono::steady_clock::now()) {
    static_assert(std::atomic_int::is_always_lock_free);
}

void Profiler::log() {
    std::chrono::duration<double, std::milli> total_frames_duration = std::chrono::milliseconds(0);
    if (jack_sample_rate > 0) {
        const double total_frames_duration_sec = static_cast<double>(jack_callback_total_frames) / static_cast<double>(jack_sample_rate);
        total_frames_duration = std::chrono::duration<double>(total_frames_duration_sec);
    }
    const std::chrono::duration<double, std::milli> jack_callback_total_duration_ms = std::chrono::nanoseconds(jack_callback_total_duration);
    double avg_duration = 0.0;
    double avg_callback_frames_duration = 0.0;
    if (jack_callback_count > 0) {
        avg_duration = jack_callback_total_duration_ms.count() / static_cast<double>(jack_callback_count);
        avg_callback_frames_duration = total_frames_duration.count() / static_cast<double>(jack_callback_count);
    }

    spdlog::info("-----------------------------------------------------------------------------");
    spdlog::info(std::format("jack_callback_count             ={}", jack_callback_count.load()));
    spdlog::info(std::format("jack_callback_total_frames      ={:L}", jack_callback_total_frames.load()));
    spdlog::info(std::format("                                 {:g}ms (avg={:g}ms)", total_frames_duration.count(), avg_callback_frames_duration));
    spdlog::info(std::format("jack_callback_total_duration    ={:g}ms (avg={:g}ms)", jack_callback_total_duration_ms.count(), avg_duration));
    spdlog::info(std::format("jack_callback_total_audio_frames={:L}", jack_callback_total_audio_frames.load()));
    spdlog::info(std::format("jack_callback_fifo_underrun     ={:L}", jack_callback_fifo_underrun.load()));
    spdlog::info(std::format("engine_samples_pushed           ={:L}", engine_samples_pushed.load()));
    reset();
}

void Profiler::periodic_log() {
    if (std::chrono::steady_clock::now() < next_log) {
        return;
    }
    next_log = std::chrono::steady_clock::now() + period;
    log();
}

void Profiler::reset() {
    jack_callback_count = 0;
    jack_callback_total_frames = 0;
    jack_callback_total_audio_frames = 0;
    jack_callback_total_duration = 0;
    jack_callback_fifo_underrun = 0;
    engine_samples_pushed = 0;
}
