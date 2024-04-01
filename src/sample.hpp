#pragma once

#include <vector>
#include <filesystem>
#include <chrono>
#include <sndfile.hh>

class SampleBlock {
    private:
        static constexpr sf_count_t BUFFER_LEN = 8192;
        class Sample& sample;
        const sf_count_t start_pos;
        const std::vector<float> buffer;
        std::unique_ptr<SampleBlock> next;
        const std::vector<float> read_buffer();
    public:
        SampleBlock(Sample& sample, sf_count_t _start_pos);
        const std::vector<float>& get_buffer() const;
        sf_count_t get_start_pos() const;
        bool is_next_loaded() const;
        bool has_next() const;
        SampleBlock& get_next();
        void unload_next();
};

class Sample {
    private:
        static constexpr std::chrono::seconds PRELOAD_TIME = std::chrono::seconds(2);
        friend SampleBlock;
        SNDFILE* snd_file;
        SF_INFO info;
        SampleBlock head;
        SampleBlock& get_last_loaded();
    public:
        Sample(const std::filesystem::path path);
        Sample(Sample&&) = default;
        Sample& operator =(Sample&&) = default;
        virtual ~Sample();
        SF_INFO get_info() const;
        SampleBlock& get_head();
        void preload(sf_count_t from);
        void unload();
};

class LoopSample: public Sample {
    private:
        const int track;
        int get_track(std::filesystem::path path) const;

    public:
        LoopSample(std::filesystem::path path);
        int get_track() const;
};

class OneShotSample: public Sample {
    private:
        const static inline std::vector<std::string> NOTE_NAMES = {"C_", "C#", "D_", "D#", "E_", "F_", "F#", "G_", "G#", "A_", "A#", "B_"};
        const uint8_t note;
        uint8_t get_note(std::filesystem::path path) const;

    public:
        OneShotSample(const std::filesystem::path path);
        uint8_t get_note() const;
};