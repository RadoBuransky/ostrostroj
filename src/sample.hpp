#pragma once

#include <filesystem>
#include <forward_list>
#include <sndfile.hh>

#define		BUFFER_LEN	32*1024

struct Buffer {
    public:
        float samples[BUFFER_LEN];
        int count;
};

class Sample {
    private:
        SNDFILE* snd_file;
        SF_INFO info;
        std::forward_list<Buffer> buffers;
        bool loaded;

        void preload();

    public:
        Sample(const std::filesystem::path path);
        virtual ~Sample();

        SF_INFO get_info() const;
        std::forward_list<Buffer>& get_buffers() const;
        void load();
        void unload();
};

class SampleReader {
    private:
        Sample sample;
        bool loop;
        std::forward_list<Buffer>::const_iterator it;

        void reset();
    public:
        SampleReader(Sample sample, bool loop);
        // TODO: Unload sample
        virtual ~SampleReader() {};

        int read(std::vector<std::vector<float>> channelBuffers, int count);
        int get_samplerate();
        int get_format();
};

class LoopSample: public Sample {
    private:
        const int track;

    public:
        LoopSample(std::filesystem::path path);
        virtual ~LoopSample() {};
        SampleReader createReader();

        int get_track() const;
};

class OneShotSample: public Sample {
    private:
        const uint8_t note;

    public:
        OneShotSample(const std::filesystem::path path);
        virtual ~OneShotSample() {};
        SampleReader createReader();

        uint8_t get_note() const;
};