#include <spdlog/spdlog.h>
#include "common.hpp"
#include "sample.hpp"

Sample::Sample(std::filesystem::path path) :
    snd_file(sf_open(path.c_str(), SFM_READ, &info)),
    buffers(),
    loaded(false) {
    if (snd_file == nullptr) {
        throw OstrostrojException(std::format("Can't open file! [{}]", path.c_str()));   
    }
    spdlog::info(std::format("File open. [{}, {} Hz, {} ch, {:x}]", path.c_str(), info.samplerate, info.channels, info.format));
};

Sample::~Sample() {
    if (snd_file != nullptr) {
        sf_close(snd_file);
        snd_file = nullptr;
    }
}

SF_INFO Sample::get_info() const {
    return info;
}

void Sample::preload() {
    auto it = buffers.cbegin();
    it++;
    it->count;
}

LoopSample::LoopSample(const std::filesystem::path path):
    Sample(path),
    track(0) { // TODO: Get track number
}

int LoopSample::get_track() const {
    return track;
}

OneShotSample::OneShotSample(const std::filesystem::path path):
    Sample(path),
    note(0) {
}

uint8_t OneShotSample::get_note() const {
    return note;
}