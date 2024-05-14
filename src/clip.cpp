#include "common.hpp"
#include <algorithm>
#include "clip.hpp"
#include "clipfx.hpp"

void FileClip::load() {
    ClipBlock* last = head.get();
    while (last->has_next()) {
        last = &last->get_next();
    }
}

FileClip::FileClip(const std::filesystem::path _path) :
    path(_path),
    snd_file(sf_open(_path.c_str(), SFM_READ, &info)) {
    if (snd_file == nullptr) {
        throw OstrostrojException(fmt::format("FCLIP can't open file! [{}]", _path.c_str()));   
    }
    if (sf_error(snd_file) != SF_ERR_NO_ERROR) {
        throw OstrostrojException(fmt::format("FCLIP file error! [{}]", sf_error(snd_file)));   
    }
    head = std::make_unique<FileClipBlock>(snd_file, info.channels, 0);
    load();
    SPDLOG_TRACE(std::format("FCLIP loaded. [{},{}Hz,{}ch,{:x}]", _path.c_str(), info.samplerate, info.channels, info.format));
};

FileClip::~FileClip() {
    if (snd_file != nullptr) {
        sf_close(snd_file);
        snd_file = nullptr;
        SPDLOG_DEBUG("FCLIP closed. [{}]", path.c_str());
    }
}

const std::filesystem::path& FileClip::get_path() const {
    return path;
}

SF_INFO& FileClip::get_info() {
    return info;
}

void FileClip::assert_sample_rate(const int expected_sample_rate) const {
    if (expected_sample_rate != info.samplerate) {
        throw OstrostrojException(fmt::format("FCLIP {}Hz sample rate expected! [{}Hz, {}]", expected_sample_rate, info.samplerate, path.string()));
    }
}

ClipBlock& FileClip::get_head() const {
    return *head;
}

LoopClip::LoopClip(const std::filesystem::path _path):
    FileClip(_path),
    track(get_track(_path)) {
  SPDLOG_DEBUG("FCLIP loop loaded [{},{},{}Hz,{}ch,{:x}]", track, _path.string(), info.samplerate, info.channels, info.format);
}

int LoopClip::get_track(std::filesystem::path _path) const {
    const auto path_filename = _path.filename().string();
    return std::stoi(path_filename.substr(1, 1)) - 1;
}

int LoopClip::get_track() const {
    return track;
}

OneShotClip::OneShotClip(const std::filesystem::path _path):
    FileClip(_path),
    note(get_note(_path)) {
  SPDLOG_DEBUG("FCLIP shot loaded [{},{},{}Hz,{}ch,{:x}]", note, _path.string(), info.samplerate, info.channels, info.format);
}

uint8_t OneShotClip::get_note(std::filesystem::path _path) const {
    const auto path_filename = _path.filename().string();
    const auto octave = std::stoi(path_filename.substr(1, 1));
    const auto note_name = path_filename.substr(2, 2);
    const unsigned int note_name_index = std::distance(NOTE_NAMES.cbegin(), std::find(NOTE_NAMES.cbegin(), NOTE_NAMES.cend(), note_name));
    if (note_name_index >= NOTE_NAMES.size()) {
        throw OstrostrojException(fmt::format("FCLIP invalid note name! [{}]", note_name));
    }
    return octave * 12 + note_name_index;
}

uint8_t OneShotClip::get_note() const {
    return note;
}