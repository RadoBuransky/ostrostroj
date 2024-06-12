#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "midi_note.hpp"

static const std::array<std::string, 12> NOTES = {{"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"}};

uint8_t MidiNote::to_value(MiniNoteName _name, uint8_t _octave) const {
    return _name + _octave*NOTES.size();
}

uint8_t MidiNote::to_value(std::string _name, uint8_t _octave) const {
    for (size_t i = 0; i < NOTES.size(); i++) {
        if (_name == NOTES.at(i)) {
            return to_value((MiniNoteName)i, _octave);
        }
    }
    throw OstrostrojException(fmt::format("NOTE  invalid note name! [_name={}]", _name));
}

std::string MidiNote::to_name(uint8_t _value) const {
    return NOTES.at(_value % NOTES.size());
}

uint8_t MidiNote::to_octave(uint8_t _value) const {
    return _value / NOTES.size();
}

MidiNote::MidiNote(MiniNoteName _name, uint8_t _octave):
    name(NOTES.at(_name)),
    octave(_octave),
    value(to_value(_name, _octave)) {
}

MidiNote::MidiNote(std::string _name, uint8_t _octave):
    name(_name),
    octave(_octave),
    value(to_value(_name, _octave)) {
}

MidiNote::MidiNote(uint8_t _value):
    name(to_name(_value)),
    octave(to_octave(_value)),
    value(_value) {
}

std::string MidiNote::get_name() const {
    return name;
}

uint8_t MidiNote::get_octave() const {
    return octave;
}

uint8_t MidiNote::get_value() const {
    return value;
}

std::string MidiNote::to_string() const {
    return name + std::to_string(octave);
}