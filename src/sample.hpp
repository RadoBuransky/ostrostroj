#pragma once

#include <array>
#include <filesystem>
#include <forward_list>
#include <iterator>
#include <sndfile.hh>

class SampleBlock {
    private:
        static constexpr int BUFFER_LEN = 32*1024;
        class Sample& sample;
        const std::vector<float> buffer;
        std::unique_ptr<SampleBlock> next;
        const std::vector<float> read_buffer();
    public:
        SampleBlock(Sample& sample);
        virtual ~SampleBlock();
        std::vector<float>& get_buffer();
        bool has_next();
        SampleBlock& get_next();
};

class Sample {
    private:
        friend SampleBlock;
        SNDFILE* snd_file;
        SF_INFO info;
        // std::unique_ptr<SampleBlock> head;
    public:
        Sample(const std::filesystem::path path);
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
        virtual ~LoopSample() {};
        int get_track() const;
};

class OneShotSample: public Sample {
    private:
        const static inline std::vector<std::string> NOTE_NAMES = {"C_", "C#", "D_", "D#", "E_", "F_", "F#", "G_", "G#", "A_", "A#", "B_"};
        const uint8_t note;
        uint8_t get_note(std::filesystem::path path) const;

    public:
        OneShotSample(const std::filesystem::path path);
        virtual ~OneShotSample() {};
        uint8_t get_note() const;
};