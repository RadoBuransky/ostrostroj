
// #define PROFILING

class Profiler {
    private:
        static constexpr std::chrono::seconds period = std::chrono::seconds(5);
        std::atomic_long next_timestamp;
        std::atomic_long last_duration_ns;
        Profiler();

    public:
        static Profiler& get() {
            static Profiler instance;
            return instance;
        }
        Profiler(Profiler const&) = delete;
        void operator=(Profiler const&) = delete;

        std::atomic_int jack_sample_rate;
        std::atomic_int jack_callback_count;
        std::atomic_int jack_callback_total_frames;
        std::atomic_int jack_callback_total_audio_frames;
        std::atomic_long jack_callback_total_duration;
        std::atomic_long jack_callback_max_duration;
        std::atomic_int jack_callback_fifo_underrun;
        std::atomic_int engine_run_count;
        std::atomic_int engine_phase_count;
        std::atomic_long engine_phase_total_duration;
        std::atomic_long engine_phase_max_duration;
        std::atomic_int engine_samples_pushed;
        std::atomic_int engine_tasks_count;
        std::atomic_int engine_tasks_in_progress;
        std::atomic_int engine_tasks_late;
        std::atomic_long clip_total_frames_read;

        std::chrono::time_point<std::chrono::steady_clock> next_log;

        void log();
        void periodic_log();
        void reset();
        void next_engine_phase();
        void tasks_done();
};