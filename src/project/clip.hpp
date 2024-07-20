#pragma once

#include "clipblock.hpp"

class Clip {
    protected:
        std::filesystem::path path;
        SNDFILE* snd_file;
        SF_INFO info;
        std::unique_ptr<FileClipBlock> head;
        size_t mem_size_bytes;
        size_t load();
    public:
        Clip(const std::filesystem::path _path);
        virtual ~Clip();
        const std::filesystem::path& get_path() const;
        SF_INFO& get_info();
        void assert_format(const int expected_sample_rate, const int expected_channels) const;
        ClipBlock& get_head() const;
        size_t get_mem_size_bytes() const;
        bool is_warp_enabled() const;
};