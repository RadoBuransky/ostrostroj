#pragma once

class Saturation {
    private:
        const uint8_t track_number;
        float dry_wet;
        float in_gain;
        float out_gain;
    public:
        Saturation(uint8_t _track_number);
        virtual ~Saturation() = default;

        void set_dry_wet(float _dry_wet);
        void set_drive(float _drive);

        float saturate(float sample);
};