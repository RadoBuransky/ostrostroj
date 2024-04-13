#include <array>
#include <spdlog/spdlog.h>
#include "clip.hpp"
#include "common.hpp"

void ClipBlock::read_buffer() {
    Clip& _clip = clip.get();
    SNDFILE* snd_file = _clip.snd_file;
    buffer_frames = sf_readf_float(snd_file, buffer.data(), buffer_capacity_frames);  
    if (sf_error(snd_file) != SF_ERR_NO_ERROR) {
        throw OstrostrojException(std::format("File error! [{}]", sf_error(snd_file)));   
    }
}

ClipBlock::ClipBlock(std::reference_wrapper<Clip>& _clip, sf_count_t _start_pos):
    clip(_clip),
    start_pos(_start_pos),
    buffer(),
    buffer_capacity_frames(buffer.size() / _clip.get().info.channels) {
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

bool ClipBlock::is_next_loaded() const {
    return has_next() && (next.get() != nullptr);
}

bool ClipBlock::has_next() const {
    return buffer_frames == buffer_capacity_frames;
}

ClipBlock& ClipBlock::get_next() {
    if (!has_next()) {
        throw OstrostrojException("No more samples!");
    }
    if (next.get() == nullptr) {
        next = std::make_unique<ClipBlock>(clip, start_pos + buffer_frames);
    }
    return *next;
}

void ClipBlock::unload_next() {
    next.reset();
}