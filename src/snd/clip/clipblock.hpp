#pragma once

#include "common.hpp"

class Clip;

typedef std::array<short, 8*8192> clip_buffer;

class ClipBlock {
    public:
        virtual ~ClipBlock() = default;
        virtual clip_buffer& get_buffer() = 0;
        virtual sf_count_t get_buffer_frames() const = 0;
        virtual bool has_next() const = 0;
        virtual ClipBlock& get_next() = 0;
        virtual int get_channels() const = 0;
        long get_total_frames();
};

class FileClipBlock: public ClipBlock {
    private:
        SNDFILE* snd_file;
        int channels;
        const sf_count_t start_pos;
        clip_buffer buffer;
        sf_count_t buffer_capacity_frames;
        sf_count_t buffer_frames;
        std::unique_ptr<FileClipBlock> next;
        void read_buffer();
    public:
        FileClipBlock(SNDFILE* _snd_file, int _channels, sf_count_t _start_pos);
        virtual ~FileClipBlock() = default;
        virtual clip_buffer& get_buffer();
        virtual sf_count_t get_buffer_frames() const;
        virtual bool has_next() const;
        virtual ClipBlock& get_next();
        virtual int get_channels() const;
};

class BufferClipBlock: public ClipBlock {
    private:
        clip_buffer buffer;
        std::unique_ptr<BufferClipBlock> next;
    public:
        BufferClipBlock(int block_count);
        virtual ~BufferClipBlock() = default;
        virtual clip_buffer& get_buffer();
        virtual sf_count_t get_buffer_frames() const;
        virtual bool has_next() const;
        virtual ClipBlock& get_next();
        virtual int get_channels() const;
};