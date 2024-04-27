#pragma once

#include <vector>
#include <filesystem>
#include <chrono>
#include <sndfile.hh>

class Clip;

typedef std::array<float, 8*8192> clip_buffer;

class ClipBlock {
    private:
        SNDFILE* snd_file;
        int channels;
        const sf_count_t start_pos;
        clip_buffer buffer;
        sf_count_t buffer_capacity_frames;
        sf_count_t buffer_frames;
        std::unique_ptr<ClipBlock> next;
        void read_buffer();
    public:
        ClipBlock(SNDFILE* _snd_file, int _channels, sf_count_t _start_pos);
        virtual ~ClipBlock() = default;
        const clip_buffer& get_buffer() const;
        sf_count_t get_buffer_frames() const;
        sf_count_t get_start_pos() const;
        bool has_next() const;
        ClipBlock& get_next();
};

class Clip {
    public:
        Clip() = default;
        virtual ~Clip() = default;
        virtual ClipBlock& get_head() = 0;
};

class FileClip: public Clip {
    protected:
        friend ClipBlock;
        std::filesystem::path path;
        SNDFILE* snd_file;
        SF_INFO info;
        std::unique_ptr<ClipBlock> head;
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