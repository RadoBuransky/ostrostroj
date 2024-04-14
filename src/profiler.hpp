#include <atomic>
#include <spdlog/spdlog.h>

class Profiler {
    private:
        static constexpr std::chrono::seconds period = std::chrono::seconds(3);
        Profiler();

    public:
        static Profiler& get() {
            static Profiler instance;
            return instance;
        }

        std::atomic_int jack_sample_rate;
        std::atomic_int jack_callback_count;
        std::atomic_int jack_callback_total_frames;
        std::atomic_int jack_callback_total_audio_frames;
        std::atomic_int jack_callback_total_duration;
        std::atomic_int jack_callback_fifo_underrun;
        std::atomic_int engine_samples_pushed;

        std::chrono::time_point<std::chrono::steady_clock> next_log;

        void log();
        void periodic_log();
        void reset();
};