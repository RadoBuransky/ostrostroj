#include <array>
#include "sample.hpp"
#include "common.hpp"

const std::vector<float> SampleBlock::read_buffer() {
    float* samples = new float[BUFFER_LEN];
    sf_count_t frames_read = sf_readf_float(sample.snd_file, samples, BUFFER_LEN / sample.info.channels);
    std::vector<float> result = std::vector<float>();
    result.assign(samples, samples + (frames_read * sample.info.channels));
    return std::move(result);
}

SampleBlock::SampleBlock(Sample& sample, sf_count_t _start_pos):
    sample(sample),
    start_pos(_start_pos),
    buffer(std::move(read_buffer())) {
    if ((BUFFER_LEN % sample.info.channels) != 0) {
        throw OstrostrojException("Invalid buffer length!");
    }
}

const std::vector<float>& SampleBlock::get_buffer() const {
    return buffer;
}

sf_count_t SampleBlock::get_start_pos() const {
    return start_pos;
}

bool SampleBlock::is_next_loaded() const {
    return has_next() && (next.get() != nullptr);
}

bool SampleBlock::has_next() const {
    return buffer.size() == BUFFER_LEN;
}

SampleBlock& SampleBlock::get_next() {
    if (!has_next()) {
        throw OstrostrojException("No more samples!");
    }
    if (next.get() == nullptr) {
        next = std::make_unique<SampleBlock>(sample, start_pos + (buffer.size() / sample.info.channels));
    }
    return *next;
}

void SampleBlock::unload_next() {
    next.reset();
}