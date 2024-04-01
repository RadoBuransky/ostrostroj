#include <array>
#include "clip.hpp"
#include "common.hpp"

const std::vector<float> ClipBlock::read_buffer() {
    float* samples = new float[BUFFER_LEN];
    sf_count_t frames_read = sf_readf_float(clip.snd_file, samples, BUFFER_LEN / clip.info.channels);
    std::vector<float> result = std::vector<float>();
    result.assign(samples, samples + (frames_read * clip.info.channels));
    return std::move(result);
}

ClipBlock::ClipBlock(Clip& clip, sf_count_t _start_pos):
    clip(clip),
    start_pos(_start_pos),
    buffer(std::move(read_buffer())) {
    if ((BUFFER_LEN % clip.info.channels) != 0) {
        throw OstrostrojException("Invalid buffer length!");
    }
}

const std::vector<float>& ClipBlock::get_buffer() const {
    return buffer;
}

sf_count_t ClipBlock::get_start_pos() const {
    return start_pos;
}

bool ClipBlock::is_next_loaded() const {
    return has_next() && (next.get() != nullptr);
}

bool ClipBlock::has_next() const {
    return buffer.size() == BUFFER_LEN;
}

ClipBlock& ClipBlock::get_next() {
    if (!has_next()) {
        throw OstrostrojException("No more samples!");
    }
    if (next.get() == nullptr) {
        next = std::make_unique<ClipBlock>(clip, start_pos + (buffer.size() / clip.info.channels));
    }
    return *next;
}

void ClipBlock::unload_next() {
    next.reset();
}