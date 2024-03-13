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
        SF_INFO sfinfo;
        std::forward_list<Buffer> buffers;
        bool loaded;

        void preload();

    public:
        Sample(std::filesystem::path path);
        ~Sample();

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
        ~SampleReader();

        int read(std::vector<std::vector<float>> channelBuffers, int count);
        int get_samplerate();
        int get_format();
};

class LoopSample {
    private:
        Sample sample;
        int track;

    public:
        LoopSample(std::filesystem::path path);
        ~LoopSample();
        SampleReader createReader();
};

class OneShotSample {
    private:
        Sample sample;
        uint8_t note;

    public:
        OneShotSample(std::filesystem::path path);
        ~OneShotSample();
        SampleReader createReader();
};