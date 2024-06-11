#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "clipblock.hpp"
#include "profiler.hpp"

long ClipBlock::get_total_frames() {
    if (has_next()) {
        return get_buffer_frames() + get_next().get_total_frames();
    }
    return get_buffer_frames();
}

void FileClipBlock::read_buffer() {
    buffer_frames = sf_readf_float(snd_file, buffer.data(), buffer_capacity_frames);  
    Profiler::get().clip_total_frames_read += buffer_frames;
    if (buffer_frames != buffer_capacity_frames) {
        SPDLOG_TRACE("{} frames read. [capacity={}]", buffer_frames, buffer_capacity_frames);
    }
    if (sf_error(snd_file) != SF_ERR_NO_ERROR) {
        throw OstrostrojException(fmt::format("File error! [{}]", sf_error(snd_file)));
    }
}

FileClipBlock::FileClipBlock(SNDFILE* _snd_file, int _channels, sf_count_t _start_pos):
    snd_file(_snd_file),
    channels(_channels),
    start_pos(_start_pos),
    buffer(),
    buffer_capacity_frames(buffer.size() / _channels) {
    read_buffer();
}

clip_buffer& FileClipBlock::get_buffer() {
    return buffer;
}

sf_count_t FileClipBlock::get_buffer_frames() const {
    return buffer_frames;
}

bool FileClipBlock::has_next() const {
    return buffer_frames == buffer_capacity_frames;
}

ClipBlock& FileClipBlock::get_next() {
    if (!has_next()) {
        throw OstrostrojException("No more samples!");
    }
    if (next.get() == nullptr) {
        next = std::make_unique<FileClipBlock>(snd_file, channels, start_pos + buffer_frames);
    }
    return *next;
}

int FileClipBlock::get_channels() const {
    return channels;
}

BufferClipBlock::BufferClipBlock(int block_count) {
    if (block_count > 1) {
        next = std::make_unique<BufferClipBlock>(block_count - 1);
    }
}

clip_buffer& BufferClipBlock::get_buffer() {
    return buffer;
}

sf_count_t BufferClipBlock::get_buffer_frames() const {    
    return buffer.size();
}

bool BufferClipBlock::has_next() const {
    return next.get() != nullptr;    
}

ClipBlock& BufferClipBlock::get_next() {  
    if (!has_next()) {
        throw OstrostrojException("No more samples!");
    }
    return *next;
}

int BufferClipBlock::get_channels() const {
    return 1;    
}