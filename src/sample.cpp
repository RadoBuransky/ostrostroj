#include <spdlog/spdlog.h>
#include "common.hpp"
#include "sample.hpp"

Sample::Sample(std::filesystem::path path) :
    snd_file(sf_open(path.c_str(), SFM_READ, &sfinfo)),
    buffers(),
    loaded(false) {
    if (snd_file == nullptr) {
        throw OstrostrojException(std::format("Can't open file! [{}]", path.c_str()));   
    }
    spdlog::info(std::format("File open. [{}, {} Hz, {} ch, {:x}]", path.c_str(), sfinfo.samplerate, sfinfo.channels, sfinfo.format));
};

Sample::~Sample() {
    if (snd_file != nullptr) {
        sf_close(snd_file);
        snd_file = nullptr;
    }
}

void Sample::preload() {
    auto it = buffers.cbegin();
    it++;
    it->count;
}

LoopSample::LoopSample(const std::filesystem::path path):
    sample(Sample(path)),
    track(0) {
}

OneShotSample::OneShotSample(const std::filesystem::path path):
    sample(Sample(path)),
    note(0) {
}

uint8_t OneShotSample::get_note() const {
    return note;
}