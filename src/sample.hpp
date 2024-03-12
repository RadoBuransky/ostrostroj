#pragma once

#include <filesystem>
#include <forward_list>
#include <sndfile.hh>

#define		BUFFER_LEN	32*1024

struct Buffer {
    public:
        double samples[BUFFER_LEN];
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
        const Sample sample;
        const bool loop;
        std::forward_list<Buffer>::const_iterator it;

        void reset();
    public:
        SampleReader(std::filesystem::path path, bool loop);

        int read(double buffer[], int count);
};