#pragma once

#include <vector>
#include <filesystem>
#include <chrono>
#include <sndfile.hh>

class Clip;

class ClipBlock {
    private:
        static constexpr sf_count_t BUFFER_LEN = 8192;
        std::reference_wrapper<Clip>& clip;
        const sf_count_t start_pos;
        std::vector<float> buffer;
        std::unique_ptr<ClipBlock> next;
        void read_buffer();
    public:
        ClipBlock(std::reference_wrapper<Clip>& clip, sf_count_t _start_pos);
        const std::vector<float>& get_buffer() const;
        sf_count_t get_start_pos() const;
        bool is_next_loaded() const;
        bool has_next() const;
        ClipBlock& get_next();
        void unload_next();
};

class Clip {
    protected:
        static constexpr std::chrono::seconds PRELOAD_TIME = std::chrono::seconds(2);
        friend ClipBlock;
        std::filesystem::path path;
        SNDFILE* snd_file;
        SF_INFO info;
        std::unique_ptr<ClipBlock> head;
        std::unique_ptr<std::reference_wrapper<Clip>> self;
        ClipBlock& get_last_loaded();
        void preload();
    public:
        Clip(const std::filesystem::path _path);
        Clip(Clip&&);
        Clip& operator =(Clip&&);
        virtual ~Clip();
        const std::filesystem::path& get_path() const;
        SF_INFO& get_info();
        void assert_sample_rate(const int expected_sample_rate) const;
        ClipBlock& get_head();
        bool load_next();
        void unload();
};

class LoopClip: public Clip {
    private:
        const int track;
        int get_track(std::filesystem::path _path) const;
    public:
        LoopClip(std::filesystem::path _path);
        int get_track() const;
};

class OneShotClip: public Clip {
    private:
        const static inline std::vector<std::string> NOTE_NAMES = {"C_", "C#", "D_", "D#", "E_", "F_", "F#", "G_", "G#", "A_", "A#", "B_"};
        const uint8_t note;
        uint8_t get_note(std::filesystem::path _path) const;

    public:
        OneShotClip(const std::filesystem::path _path);
        uint8_t get_note() const;
};