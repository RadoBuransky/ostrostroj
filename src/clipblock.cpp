#include <array>
#include <spdlog/spdlog.h>
#include "clip.hpp"
#include "common.hpp"

void ClipBlock::read_buffer() {
    spdlog::trace(std::format("{}", __FUNCTION__));
    Clip& _clip = clip.get();
    buffer.reserve(BUFFER_LEN);
    spdlog::debug(std::format("Reading... [{}]", _clip.info.channels));
    SNDFILE* snd_file = _clip.snd_file;
    sf_count_t frames_read = sf_readf_float(snd_file, &(*buffer.begin()), BUFFER_LEN / _clip.info.channels);  
    if (sf_error(snd_file) != SF_ERR_NO_ERROR) {
        throw OstrostrojException(std::format("File error! [{}]", sf_error(snd_file)));   
    }
    buffer.resize(frames_read * _clip.info.channels);

    int samples = 0;
    double buffer_energy = 0.0;
    for (const float& sample : buffer) {
        samples++;
        buffer_energy += std::abs(sample);
    }

    sf_count_t offset = sf_seek(snd_file, 0, SEEK_CUR);
    spdlog::debug(std::format("Read. [frames_read={}, samples={}, energy={:g}, offset={}, path={}]", frames_read, samples, buffer_energy, offset, _clip.path.c_str()));
}

ClipBlock::ClipBlock(std::reference_wrapper<Clip>& _clip, sf_count_t _start_pos):
    clip(_clip),
    start_pos(_start_pos),
    buffer() {
    read_buffer();
    spdlog::trace(std::format("{}(_clip=0x{:x})", __FUNCTION__, reinterpret_cast<intptr_t>(&_clip)));
    if ((BUFFER_LEN % _clip.get().info.channels) != 0) {
        throw OstrostrojException(std::format("Invalid buffer length! [{}, {}, {}]", _clip.get().get_path().c_str(), BUFFER_LEN, _clip.get().info.channels));
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
        spdlog::trace(std::format("get_next make_unique({}, {})", clip.get().get_path().c_str(), clip.get().info.channels));
        next = std::make_unique<ClipBlock>(clip, start_pos + (buffer.size() / clip.get().info.channels));
    }
    spdlog::trace(std::format("{} done", __FUNCTION__));
    return *next;
}

void ClipBlock::unload_next() {
    next.reset();
}