#define SPDLOG_ACTIVE_LEVEL 1

#include "common.hpp"
#include "warp.hpp"

static constexpr uint8_t CONVERTER = SRC_LINEAR;
static constexpr double TARGET_CHANGE_PERIOD_SEC = 3;
static constexpr double MAX_RATIO = 0.01;
static constexpr double MAX_STEP = 0.0000001;

void Warp::update_ratio() {
    if (target_change_dist(random_engine) == 1) {
        step_size = generate_step_size();
        if (track_number == 4) {
            SPDLOG_DEBUG("WARP{}  update_ratio[step_size={:.3f},ratio_accumulator={:.3f},ratio={:.3f}]", track_number, step_size, ratio_accumulator, ratio);
        }
    }
    ratio += step_size;
    if (ratio > MAX_RATIO) {
        ratio = MAX_RATIO;
    } else {
        if (ratio < -MAX_RATIO) {
            ratio = -MAX_RATIO;
        }
    }
    src_data.src_ratio = (ratio >= 0.0) ? (1.0 + ratio) : (1.0 / (1.0 - ratio));
}

double Warp::generate_step_size() {
    double result = step_dist(random_engine) - (ratio_accumulator * MAX_STEP * 1000.0);
    if (result > MAX_STEP) {
        return MAX_STEP;
    } 
    if (result < -MAX_STEP) {
        return -MAX_STEP;
    }
    return result;
}

SRC_STATE* Warp::init_src_state(size_t _channels) {
    int error;
    SRC_STATE* result = src_new(CONVERTER, _channels, &error);
    if (result == nullptr) {
        throw OstrostrojException(fmt::format("WARP src_new failed! [error={}]", src_strerror(error)));
    }
    SPDLOG_DEBUG("WARP  initialized [converter={},channels={}]", CONVERTER, _channels);
    return result;
}

Warp::Warp(size_t _channels, uint8_t _track_number):
    channels(_channels),
    track_number(_track_number),
    input_samples(),
    input_samples_pos(input_samples.data()),
    output_samples_gen(0),
    src_state(init_src_state(_channels)),
    ratio(0.0),
    ratio_accumulator(0.0),
    step_size(generate_step_size()),
    random(),
    random_engine(random()),
    target_change_dist(1, (uint)(96000 * _channels * TARGET_CHANGE_PERIOD_SEC / input_samples.size())), // Yeah, hardcoded sampling rate
    step_dist(-MAX_STEP, MAX_STEP) {
    if (channels*2 > input_samples.size()) {
        throw OstrostrojException(fmt::format("WARP{} we need to fit at least 2 frames in input buffer! [channels={}]", _track_number, channels));
    }
    src_data.data_in = input_samples.data();
    src_data.input_frames = input_samples.size() / channels;
    src_data.input_frames_used = 0;
    src_data.data_out = output_samples.data();
    src_data.output_frames = output_samples.size() / channels;
    src_data.output_frames_gen = 0;
    src_data.src_ratio = 1.0;
    src_data.end_of_input = 0;
    update_ratio();
}

Warp::~Warp() {
    if (src_state != nullptr) {
        src_delete(src_state);
        src_state = nullptr;
    }
}

bool Warp::pushnpop(float &sample) {
    if (output_samples_gen > 0) {
        throw OstrostrojException(fmt::format("WARP illegal push! [output_samples_gen={}]", output_samples_gen));
    }
    *input_samples_pos = sample;
    input_samples_pos++;
    if (input_samples_pos == input_samples.end()) {
        int error;
        src_data.data_in = input_samples.data();
        src_data.input_frames = input_samples.size() / channels;
        src_data.input_frames_used = 0;
        src_data.data_out = output_samples.data();
        src_data.output_frames = output_samples.size() / channels;
        src_data.output_frames_gen = 0;
        src_data.end_of_input = 0;

        input_samples_pos = input_samples.data();

        if ((error = src_process(src_state, &src_data))) {
            throw OstrostrojException(fmt::format("WARP src_process failed! [error={}]", src_strerror(error)));
        }
        if ((size_t)src_data.input_frames_used != input_samples.size() / channels) {
            throw OstrostrojException(fmt::format("WARP didn't use all input frames! [input_frames_used={},output_frames_gen={}]",
                src_data.input_frames_used, src_data.output_frames_gen));
        }
        output_samples_gen = src_data.output_frames_gen * channels;
        ratio_accumulator += output_samples_gen * ratio;
        update_ratio();
    }

    return pop(sample);
}

bool Warp::pop(float &sample) {
    if (output_samples_gen > 0) {
        sample = *src_data.data_out;
        src_data.data_out++;
        output_samples_gen--;
        return true;
    }
    return false;
}