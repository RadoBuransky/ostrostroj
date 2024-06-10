#include "common.hpp"
#include "midi_encoder.hpp"

MidiEncoder::MidiEncoder(uint8_t _channel, uint _param, int _min_value, int _max_value):
    channel(_channel),
    param(_param),
    min_value(_min_value),
    max_value(_max_value),
    value(INT_MIN),
    prev_value(INT_MIN),
    grabbed(false) {
    if (min_value <= max_value) {
        throw OstrostrojException(fmt::format("CNTRL invalid min max [min={},max={}]", min_value, max_value));
    }
}

bool MidiEncoder::handle(uint8_t _channel, uint _param, int _value) {
    if ((channel != _channel) || (param != _param)) {
        return false;
    }
    if (grabbed) {
        value = _value;
        return true;
    }
    if (value == INT_MIN || _value == value) {
        value = _value;
        grabbed = true;
        return true;
    }
    if (prev_value == INT_MIN) {
        prev_value = _value;
        return true;
    }
    int start = std::min(prev_value, _value);
    int end = std::max(prev_value, _value);
    if (start <= value && value <= end) {
        grabbed = true;
        value = _value;
        return true;
    }
    prev_value = _value;
    return true;
}

int MidiEncoder::get_max_value() {
    return max_value;
}

void MidiEncoder::set_value(int _value) {
    value = _value;
    prev_value = INT_MIN;
    grabbed = false;
}

int MidiEncoder::get_value() {
    return std::max(min_value, std::min(max_value, value));
}

float MidiEncoder::get_percentage() {
    return (value - min_value) / (max_value - min_value);
}