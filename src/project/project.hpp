#pragma once

#include <vector>
#include <map>
#include "clip.hpp"
#include "song.hpp"

class Project {
    private:
        uint8_t number;
        std::string name;
        std::vector<Song> songs;
        uint8_t parse_number(std::filesystem::path dir);
        std::string parse_name(std::filesystem::path dir);
        std::vector<Song> init_songs(std::filesystem::path dir);
    public:
        Project(std::filesystem::path _dir);
        virtual ~Project() = default;
        uint8_t get_number();
        std::string get_name();
        std::vector<Song>& get_songs();
};