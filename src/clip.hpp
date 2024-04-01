#pragma once

#include <vector>
#include <filesystem>
#include <chrono>
#include <sndfile.hh>

class ClipBlock {
    private:
        static constexpr sf_count_t BUFFER_LEN = 8192;
        class Clip& clip;
        const sf_count_t start_pos;
        const std::vector<float> buffer;
        std::unique_ptr<ClipBlock> next;
        const std::vector<float> read_buffer();
    public:
        ClipBlock(Clip& clip, sf_count_t _start_pos);
        const std::vector<float>& get_buffer() const;
        sf_count_t get_start_pos() const;
        bool is_next_loaded() const;
        bool has_next() const;
        ClipBlock& get_next();
        void unload_next();
};

class Clip {
    private:
        static constexpr std::chrono::seconds PRELOAD_TIME = std::chrono::seconds(2);
        friend ClipBlock;
        SNDFILE* snd_file;
        SF_INFO info;
        ClipBlock head;
        ClipBlock& get_last_loaded();
    public:
        Clip(const std::filesystem::path path);
        Clip(Clip&&) = default;
        Clip& operator =(Clip&&) = default;
        virtual ~Clip();
        SF_INFO get_info() const;
        ClipBlock& get_head();
        void preload(sf_count_t from);
        void unload();
};

class LoopClip: public Clip {
    private:
        const int track;
        int get_track(std::filesystem::path path) const;

    public:
        LoopClip(std::filesystem::path path);
        int get_track() const;
};

class OneShotClip: public Clip {
    private:
        const static inline std::vector<std::string> NOTE_NAMES = {"C_", "C#", "D_", "D#", "E_", "F_", "F#", "G_", "G#", "A_", "A#", "B_"};
        const uint8_t note;
        uint8_t get_note(std::filesystem::path path) const;

    public:
        OneShotClip(const std::filesystem::path path);
        uint8_t get_note() const;
};