#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "saturation.hpp"

static constexpr float DRIVE = 100.0;
static constexpr float CURVE = 1000000.0;

Saturation::Saturation(uint8_t _track_number):
    track_number(_track_number),
    dry_wet(0.0),
    in_gain(0.0),
    out_gain(0.0) {
    set_dry_wet(0.0);
    set_drive(0.0);
}

void Saturation::set_dry_wet(float _dry_wet) {
    dry_wet = std::max(0.0f, std::min(1.0f, _dry_wet));
    SPDLOG_DEBUG("SAT{}  set_dry_wet[dry_wet={}]", track_number, dry_wet);
}

float Saturation::get_dry_wet() {
    return dry_wet;
}

void Saturation::set_drive(float _drive) {
    _drive = std::max(0.0f, std::min(1.0f, _drive));
    _drive = (std::pow(CURVE, _drive) - 1.0) / (CURVE - 1.0);
    in_gain = 1.0 + (_drive * DRIVE);
    out_gain = 2 / (1 + 2*std::log10(in_gain));
    SPDLOG_DEBUG("SAT{}  set_drive[_drive={},in_gain={},out_gain={}]", track_number, _drive, in_gain, out_gain);
}

float Saturation::saturate(float sample) {
    if (dry_wet == 0.0) {
        return sample;
    }
    float wet_sample = sample * in_gain;
    wet_sample = out_gain * (wet_sample / (std::abs(wet_sample) + 1.0));
    return sample*(1.0 - dry_wet) + wet_sample*dry_wet;
}