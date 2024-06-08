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

void FileClip::assert_format(const int expected_sample_rate, const int expected_channels) const {
    if (expected_sample_rate != info.samplerate) {
        throw OstrostrojException(fmt::format("FCLIP {}Hz sample rate expected! [{}Hz, {}]", expected_sample_rate, info.samplerate, path.string()));
    }
    if ((info.format & SF_FORMAT_WAV) == 0) {
        throw OstrostrojException(fmt::format("PRJKT WAV file expected! [0x{:x}, {}]", info.format, path.string()));
    }
    if ((info.format & SF_FORMAT_FLOAT) == 0) {
        throw OstrostrojException(fmt::format("PRJKT 32-bit float expected! [0x{:x}, {}]", info.format, path.string()));
    }
    if (info.channels != expected_channels) {
        throw OstrostrojException(fmt::format("PRJKT {} channels expected! [{}, {}]", expected_channels, info.channels, path.string()));
    }
}

ClipBlock& FileClip::get_head() const {
    return *head;
}