#include <algorithm>
#include <spdlog/spdlog.h>
#include "common.hpp"
#include "clip.hpp"

ClipBlock& Clip::get_last_loaded() {
    ClipBlock* result = head.get();
    while (result->is_next_loaded()) {
        result = &result->get_next();
    }
    return *result;
}

void Clip::preload() {
    ClipBlock* last = head.get();
    const sf_count_t preload_frames = info.samplerate * PRELOAD_TIME.count();
    while ((last->get_start_pos() < preload_frames) && last->has_next()) {
        last = &last->get_next();
    }
}

Clip::Clip(const std::filesystem::path _path) :
    path(_path),
    snd_file(sf_open(_path.c_str(), SFM_READ, &info)) {
    if (snd_file == nullptr) {
        throw OstrostrojException(std::format("Can't open file! [{}]", _path.c_str()));   
    }
    if (sf_error(snd_file) != SF_ERR_NO_ERROR) {
        throw OstrostrojException(std::format("File error! [{}]", sf_error(snd_file)));   
    }
    head = std::make_unique<ClipBlock>(snd_file, info.channels, 0);
    preload();
    spdlog::trace(std::format("File preloaded. [{}, {} Hz, {} ch, {:x}]", _path.c_str(), info.samplerate, info.channels, info.format));
};

Clip::Clip(Clip&& other):
    path(other.path),
    snd_file(other.snd_file),
    info(other.info),
    head(std::move(other.head)) {
    other.path.clear();
    other.snd_file = nullptr;
    other.info = {};
}

Clip& Clip::operator =(Clip&& other) {    
    path = other.path;
    other.path.clear();
    snd_file = other.snd_file;
    other.snd_file = nullptr;
    info = other.info;
    other.info = {};
    head = std::move(other.head);
    return *this;
}

Clip::~Clip() {
    if (snd_file != nullptr) {
        sf_close(snd_file);
        snd_file = nullptr;
        spdlog::debug(std::format("Clip closed. [{}]", path.c_str()));
    }
}

const std::filesystem::path& Clip::get_path() const {
    return path;
}

SF_INFO& Clip::get_info() {
    return info;
}

void Clip::assert_sample_rate(const int expected_sample_rate) const {
    if (expected_sample_rate != info.samplerate) {
        throw OstrostrojException(std::format("{}Hz sample rate expected! [{}Hz, {}]", expected_sample_rate, info.samplerate, path.string()));
    }
}

ClipBlock& Clip::get_head() {
    return *head;
}

bool Clip::load_next() {
    ClipBlock* last = &get_last_loaded();
    if (last->has_next()) {
        last->get_next();
        return true;
    }
    spdlog::info(std::format("Clip loaded completely. [{}]", path.c_str()));
    return false;
}

void Clip::unload() {
    ClipBlock* block = head.get();
    ClipBlock* prev = block;
    const sf_count_t preload_end_pos = info.samplerate * PRELOAD_TIME.count();
    while ((block->get_start_pos() < preload_end_pos) && block->is_next_loaded()) {
        prev = block;
        block = &block->get_next();
    }
    if (prev != block && block->get_start_pos() >= preload_end_pos) {
        prev->unload_next();
        // TODO: We need to reset file position.
    }
}

LoopClip::LoopClip(const std::filesystem::path _path):
    Clip(_path),
    track(get_track(_path)) {
  spdlog::info(std::format("Loop sample loaded. [{}, {}, {} Hz, {} ch, {:x}]", track, _path.string(), info.samplerate, info.channels, info.format));
}

int LoopClip::get_track(std::filesystem::path _path) const {
    const auto path_filename = _path.filename().string();
    return std::stoi(path_filename.substr(1, 1)) - 1;
}

int LoopClip::get_track() const {
    return track;
}

OneShotClip::OneShotClip(const std::filesystem::path _path):
    Clip(_path),
    note(get_note(_path)) {
  spdlog::info(std::format("One-shot sample loaded. [{}, {}, {} Hz, {} ch, {:x}]", note, _path.string(), info.samplerate, info.channels, info.format));    
}

uint8_t OneShotClip::get_note(std::filesystem::path _path) const {
    const auto path_filename = _path.filename().string();
    const auto octave = std::stoi(path_filename.substr(1, 1));
    const auto note_name = path_filename.substr(2, 2);
    const unsigned int note_name_index = std::distance(NOTE_NAMES.cbegin(), std::find(NOTE_NAMES.cbegin(), NOTE_NAMES.cend(), note_name));
    if (note_name_index >= NOTE_NAMES.size()) {
        throw OstrostrojException(std::format("Invalid note name! [{}]", note_name));
    }
    return octave * 12 + note_name_index;
}

uint8_t OneShotClip::get_note() const {
    return note;
}