#pragma once

#include "clipblock.hpp"

class Clip {
    protected:
        std::filesystem::path path;
        SNDFILE* snd_file;
        SF_INFO info;
        std::unique_ptr<FileClipBlock> head;
        void load();
    public:
        Clip(const std::filesystem::path _path);
        virtual ~Clip();
        const std::filesystem::path& get_path() const;
        SF_INFO& get_info();
        void assert_format(const int expected_sample_rate, const int expected_channels) const;
        ClipBlock& get_head() const;
};