#pragma once

class MidiEncoder {
    private:
        const uint8_t channel;
        const uint param;
        int min_value;
        int max_value;
        int value;
        int prev_value;
        bool grabbed;
    public:
        MidiEncoder(uint8_t _channel, uint _param, int _min_value, int _max_value);
        virtual ~MidiEncoder() = default;
        bool handle(uint8_t _channel, uint _param, int _value);
        int get_max_value();
        void set_value(int _value);
        int get_value();
        float get_percentage();
        void set_percentage(float _percentage);
        bool is_grabbed();
};