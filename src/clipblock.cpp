#include <array>
#include <spdlog/spdlog.h>
#include "clip.hpp"
#include "common.hpp"

const std::vector<float> ClipBlock::read_buffer() {
    spdlog::trace(std::format("{}", __FUNCTION__));
    std::vector<float> result = std::vector<float>();
    result.reserve(BUFFER_LEN);
    spdlog::trace(std::format("Reading... [{}]", clip.info.channels));
    sf_count_t frames_read = sf_readf_float(clip.snd_file, &(*result.begin()), BUFFER_LEN / clip.info.channels);  
    spdlog::trace(std::format("Read. [{}]", frames_read));  
    result.resize(frames_read * clip.info.channels);
    spdlog::trace(std::format("{} done", __FUNCTION__));
    return result;
}

ClipBlock::ClipBlock(Clip& _clip, sf_count_t _start_pos):
    clip(_clip),
    start_pos(_start_pos),
    buffer(std::move(read_buffer())) {        
    spdlog::trace(std::format("{}", __FUNCTION__));
    if ((BUFFER_LEN % _clip.info.channels) != 0) {
        throw OstrostrojException(std::format("Invalid buffer length! [{}, {}, {}]", _clip.get_path().c_str(), BUFFER_LEN, _clip.info.channels));
    }
}

const std::vector<float>& ClipBlock::get_buffer() const {
    return buffer;
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
    return buffer.size() == BUFFER_LEN;
}

ClipBlock& ClipBlock::get_next() {
    spdlog::trace(std::format("{}", __FUNCTION__));
    if (!has_next()) {
        throw OstrostrojException("No more samples!");
    }
    if (next.get() == nullptr) {
        spdlog::trace(std::format("get_next make_unique({}, {})", clip.get_path().c_str(), clip.info.channels));
        next = std::make_unique<ClipBlock>(clip, start_pos + (buffer.size() / clip.info.channels));
    }
    spdlog::trace(std::format("{} done", __FUNCTION__));
    return *next;
}

void ClipBlock::unload_next() {
    next.reset();
}