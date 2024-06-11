#include "common.hpp"
#include "loop_encoders.hpp"

LoopEncoders::LoopEncoders(uint8_t channel, uint first_param):
    encoders {
        MidiEncoder(channel, first_param + 0, 0, 127),
        MidiEncoder(channel, first_param + 1, 0, 127),
        MidiEncoder(channel, first_param + 2, 0, 127),
        MidiEncoder(channel, first_param + 3, 0, 127),
        MidiEncoder(channel, first_param + 4, 0, 127),
        MidiEncoder(channel, first_param + 5, 0, 127)
    } {    
}

ssize_t LoopEncoders::handle(uint8_t _channel, uint _param, int _value) {
    for (MidiEncoder& encoder : encoders) {
        if (encoder.handle(_channel, _param, _value)) {
            // Track number
            return (&encoder - encoders.data()) + 1;
        }
    }
    return -1;
}

MidiEncoder& LoopEncoders::get_encoder(size_t track_number) {
    return encoders.at(track_number - 1);
}