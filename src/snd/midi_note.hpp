#pragma once

enum MiniNoteName {
    C = 0,
    C_,
    D,
    D_,
    E,
    F,
    F_,
    G,
    G_,
    A,
    A_,
    B
};

class MidiNote {
    private:
        const std::string name;
        const uint8_t octave;
        const uint8_t value;
        uint8_t to_value(MiniNoteName _name, uint8_t _octave) const;
        uint8_t to_value(std::string _name, uint8_t _octave) const;
        std::string to_name(uint8_t _value) const;
        uint8_t to_octave(uint8_t _value) const;
    public:
        MidiNote(MiniNoteName _name, uint8_t _octave);
        MidiNote(std::string _name, uint8_t _octave);
        MidiNote(uint8_t _value);
        virtual ~MidiNote() = default;
        std::string get_name() const;
        uint8_t get_octave() const;
        uint8_t get_value() const;
        std::string to_string() const;
};