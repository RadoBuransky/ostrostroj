#pragma once

#include <vector>
#include <filesystem>
#include <chrono>
#include <sndfile.hh>
#include "clipblock.hpp"

class Clip {
    public:
        Clip() = default;
        virtual ~Clip() = default;
        virtual ClipBlock& get_head() const = 0;
};

class FileClip: public Clip {
    protected:
        std::filesystem::path path;
        SNDFILE* snd_file;
        SF_INFO info;
        std::unique_ptr<FileClipBlock> head;
        void load();
    public:
        FileClip(const std::filesystem::path _path);
        virtual ~FileClip();
        const std::filesystem::path& get_path() const;
        SF_INFO& get_info();
        void assert_format(const int expected_sample_rate, const int expected_channels) const;
        ClipBlock& get_head() const;
};