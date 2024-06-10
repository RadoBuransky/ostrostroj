#pragma once

#include "midi_encoder.hpp"

class LoopEncoders {
    private:
        std::array<MidiEncoder, ENGINE_LOOP_TRACKS> encoders;
    public:
        LoopEncoders(uint8_t channel, uint first_param);
        virtual ~LoopEncoders() = default;
        bool handle(uint8_t _channel, uint _param, int _value);
        MidiEncoder& get_encoder(uint8_t track_number);
};