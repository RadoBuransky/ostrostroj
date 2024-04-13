#include <array>
#include <spdlog/spdlog.h>
#include "clip.hpp"
#include "common.hpp"

void ClipBlock::read_buffer() {
    spdlog::trace(std::format("{}", __FUNCTION__));
    Clip& _clip = clip.get();
    spdlog::debug(std::format("Reading... [{}]", _clip.info.channels));
    SNDFILE* snd_file = _clip.snd_file;
    buffer_frames = sf_readf_float(snd_file, buffer.data(), buffer_capacity_frames);  
    if (sf_error(snd_file) != SF_ERR_NO_ERROR) {
        throw OstrostrojException(std::format("File error! [{}]", sf_error(snd_file)));   
    }

    double buffer_energy = 0.0;
    for (int i = 0; i < buffer_frames*_clip.info.channels; i++) {
        buffer_energy += std::abs(buffer[i]);
    }

    sf_count_t offset = sf_seek(snd_file, 0, SEEK_CUR);
    spdlog::debug(std::format("Read. [buffer_frames={}, energy={:g}, offset={}, path={}]",
        buffer_frames, buffer_energy, offset, _clip.path.c_str()));
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
    spdlog::trace(std::format("{}", __FUNCTION__));
    return has_next() && (next.get() != nullptr);
}

bool ClipBlock::has_next() const {
    spdlog::trace(std::format("{}", __FUNCTION__));
    return buffer_frames == buffer_capacity_frames;
}

ClipBlock& ClipBlock::get_next() {
    spdlog::trace(std::format("{}", __FUNCTION__));
    if (!has_next()) {
        throw OstrostrojException("No more samples!");
    }
    if (next.get() == nullptr) {
        spdlog::trace(std::format("get_next make_unique({}, {})", clip.get().get_path().c_str(), clip.get().info.channels));
        next = std::make_unique<ClipBlock>(clip, start_pos + buffer_frames);
    }
    spdlog::trace(std::format("{} done", __FUNCTION__));
    return *next;
}

void ClipBlock::unload_next() {
    next.reset();
}