#pragma once

#include <vector>
#include <filesystem>
#include <chrono>
#include <sndfile.hh>
#include "clipblock.hpp"

class Clip {
    public:
        Clip() = default;
        virtual ~Clip() = default;
        virtual ClipBlock& get_head() = 0;
};

class FileClip: public Clip {
    protected:
        std::filesystem::path path;
        SNDFILE* snd_file;
        SF_INFO info;
        std::unique_ptr<FileClipBlock> head;
        void load();
    public:
        FileClip(const std::filesystem::path _path);
        virtual ~FileClip();
        const std::filesystem::path& get_path() const;
        SF_INFO& get_info();
        void assert_sample_rate(const int expected_sample_rate) const;
        ClipBlock& get_head();
};

class LoopClip: public FileClip {
    private:
        const int track;
        int get_track(std::filesystem::path _path) const;
    public:
        LoopClip(std::filesystem::path _path);
        virtual ~LoopClip() = default;
        int get_track() const;
};

class OneShotClip: public FileClip {
    private:
        const static inline std::vector<std::string> NOTE_NAMES = {"C_", "C#", "D_", "D#", "E_", "F_", "F#", "G_", "G#", "A_", "A#", "B_"};
        const uint8_t note;
        uint8_t get_note(std::filesystem::path _path) const;

    public:
        OneShotClip(const std::filesystem::path _path);
        virtual ~OneShotClip() = default;
        uint8_t get_note() const;
};