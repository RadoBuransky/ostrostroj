#include <algorithm>
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
    spdlog::debug(std::format("File open. [{}, {} Hz, {} ch, {:x}]", path.c_str(), info.samplerate, info.channels, info.format));
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
    track(get_track(path)) {
  spdlog::info(std::format("Loop sample loaded. [{}, {}]", track, path.string()));    
}

int LoopSample::get_track(std::filesystem::path path) const {
    const auto path_filename = path.filename().string();
    return std::stoi(path_filename.substr(1, 1));
}

int LoopSample::get_track() const {
    return track;
}

OneShotSample::OneShotSample(const std::filesystem::path path):
    Sample(path),
    note(get_note(path)) {
  spdlog::info(std::format("One-shot sample loaded. [{}, {}]", note, path.string()));    
}

uint8_t OneShotSample::get_note(std::filesystem::path path) const {
    const auto path_filename = path.filename().string();
    const auto octave = std::stoi(path_filename.substr(1, 1));
    const auto note_name = path_filename.substr(2, 2);
    const auto note_name_index = std::distance(NOTE_NAMES.cbegin(), std::find(NOTE_NAMES.cbegin(), NOTE_NAMES.cend(), note_name));
    if (note_name_index >= NOTE_NAMES.size()) {
        throw OstrostrojException(std::format("Invalid note name! [{}]", note_name));
    }
    return octave * 12 + note_name_index;
}

uint8_t OneShotSample::get_note() const {
    return note;
}