#include "common.hpp"
#include "clip.hpp"
#include "profiler.hpp"

void ClipBlock::read_buffer() {
    buffer_frames = sf_readf_float(snd_file, buffer.data(), buffer_capacity_frames);  
    Profiler::get().clip_total_frames_read += buffer_frames;
    if (buffer_frames != buffer_capacity_frames) {
        SPDLOG_WARN("{} frames read. [capacity={}]", buffer_frames, buffer_capacity_frames);
    }
    if (sf_error(snd_file) != SF_ERR_NO_ERROR) {
        throw OstrostrojException(fmt::format("File error! [{}]", sf_error(snd_file)));
    }
}

ClipBlock::ClipBlock(SNDFILE* _snd_file, int _channels, sf_count_t _start_pos):
    snd_file(_snd_file),
    channels(_channels),
    start_pos(_start_pos),
    buffer(),
    buffer_capacity_frames(buffer.size() / _channels) {
    read_buffer();
}

const clip_buffer& ClipBlock::get_buffer() const {
    return buffer;
}

sf_count_t ClipBlock::get_buffer_frames() const {
    return buffer_frames;
}

sf_count_t ClipBlock::get_start_pos() const {
    return start_pos;
}

bool ClipBlock::has_next() const {
    return buffer_frames == buffer_capacity_frames;
}

ClipBlock& ClipBlock::get_next() {
    if (!has_next()) {
        throw OstrostrojException("No more samples!");
    }
    if (next.get() == nullptr) {
        next = std::make_unique<ClipBlock>(snd_file, channels, start_pos + buffer_frames);
    }
    return *next;
}