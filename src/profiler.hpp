#include <atomic>

class Profiler {
    private:
        Profiler():
            jack_callback_count(0),
            jack_callback_total_duration(0),
            jack_callback_fifo_underrun(0) {
            static_assert(std::atomic_int::is_always_lock_free);
        }

    public:
        static Profiler& get() {
            static Profiler instance;
            return instance;
        }

        std::atomic_int jack_callback_count;
        std::atomic_int jack_callback_total_duration;
        std::atomic_int jack_callback_fifo_underrun;
};