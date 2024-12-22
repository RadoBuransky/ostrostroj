#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 1
#include <spdlog/spdlog.h>
#include "clip.hpp"

size_t Clip::load() {
    ClipBlock* last = head.get();
    size_t result = last->get_buffer().size();
    while (last->has_next()) {
        last = &last->get_next();
        result += last->get_buffer().size();
    }
    return result*sizeof(float);
}

Clip::Clip(const std::filesystem::path _path) :
    path(_path),
    snd_file(sf_open(_path.c_str(), SFM_READ, &info)) {
    if (snd_file == nullptr) {
        throw OstrostrojException(fmt::format("FCLIP can't open file! [{}]", _path.c_str()));   
    }
    if (sf_error(snd_file) != SF_ERR_NO_ERROR) {
        throw OstrostrojException(fmt::format("FCLIP file error! [{}]", sf_error(snd_file)));   
    }
    head = std::make_unique<FileClipBlock>(snd_file, info.channels, 0);
    mem_size_bytes = load();
    SPDLOG_DEBUG(fmt::format("FCLIP loaded. [{},{}Hz,{}ch,{:x}]", _path.c_str(), info.samplerate, info.channels, info.format));
};

Clip::~Clip() {
    if (snd_file != nullptr) {
        sf_close(snd_file);
        snd_file = nullptr;
        SPDLOG_DEBUG("FCLIP closed. [{}]", path.c_str());
    }
}

const std::filesystem::path& Clip::get_path() const {
    return path;
}

SF_INFO& Clip::get_info() {
    return info;
}

void Clip::assert_format(const int expected_sample_rate, const int expected_channels) const {
    if (expected_sample_rate > 0 && expected_sample_rate != info.samplerate) {
        throw OstrostrojException(fmt::format("FCLIP {}Hz sample rate expected! [{}Hz, {}]", expected_sample_rate, info.samplerate, path.string()));
    }
    if ((info.format & SF_FORMAT_WAV) == 0) {
        throw OstrostrojException(fmt::format("FCLIP WAV file expected! [0x{:x}, {}]", info.format, path.string()));
    }
    if ((info.format & SF_FORMAT_PCM_16) == 0) {
        throw OstrostrojException(fmt::format("FCLIP 16-bit short expected! [0x{:x}, {}]", info.format, path.string()));
    }
    if (info.channels != expected_channels) {
        throw OstrostrojException(fmt::format("FCLIP {} channels expected! [{}, {}]", expected_channels, info.channels, path.string()));
    }
}

ClipBlock& Clip::get_head() const {
    return *head;
}

size_t Clip::get_mem_size_bytes() const {
    return mem_size_bytes;
}

bool Clip::is_warp_enabled() const {
    return path.filename().string().find("_w.") != std::string::npos;
}