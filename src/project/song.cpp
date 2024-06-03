#include "common.hpp"
#include "song.hpp"

Song::Song(const std::filesystem::path _dir):
    dir(_dir),
    root_bank_pattern(_dir.filename()) { // TODO: Parse dir name
}

std::vector<Pattern2>& Song::get_patterns() {
    return patterns;
}

std::vector<SongOneShot>& Song::get_one_shots() {    
    return one_shots;
}