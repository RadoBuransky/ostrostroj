#include "common.hpp"
#include "warp.hpp"

static constexpr uint8_t CONVERTER = SRC_LINEAR;

SRC_STATE* Warp::init_src_state(size_t _channels) {
    int error;
    SRC_STATE* result = src_new(CONVERTER, _channels, &error);
    if (result == nullptr) {
        throw OstrostrojException(fmt::format("WARP src_new failed! [error={}]", src_strerror(error)));
    }
    SPDLOG_DEBUG("WARP  initialized [converter={},channels={}]", CONVERTER, _channels);
    return result;
}

Warp::Warp(size_t _channels):
    channels(_channels),
    input_frame_pos(0),
    output_samples_gen(0),
    src_state(init_src_state(_channels)),
    ratio(0.7) {
    if (channels > input_frame.size()) {
        throw OstrostrojException(fmt::format("WARP too many channels! [channels={}]", channels));
    }
    src_data.data_in = input_frame.data();
    src_data.input_frames = 1;
    src_data.input_frames_used = 0;
    src_data.data_out = output.data();
    src_data.output_frames = output.size() / channels;
    src_data.output_frames_gen = 0;
    src_data.src_ratio = 1.0;
    src_data.end_of_input = 0;
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

    input_frame.at(input_frame_pos) = sample;
    input_frame_pos++;

    if (input_frame_pos == channels) {
        int error;

        input_frame_pos = 0;
        src_data.src_ratio = ratio;
        src_data.data_in = input_frame.data();
        src_data.input_frames = 1;
        src_data.input_frames_used = 0;
        src_data.data_out = output.data();
        src_data.output_frames = output.size();
        src_data.output_frames_gen = 0;
        src_data.end_of_input = 0;

        if ((error = src_process(src_state, &src_data))) {
            throw OstrostrojException(fmt::format("WARP src_process failed! [error={}]", src_strerror(error)));
        }
        if (src_data.input_frames_used != 1) {
            throw OstrostrojException(fmt::format("WARP didn't use input frame! [input_frames_used={},output_frames_gen={}]",
                src_data.input_frames_used, src_data.output_frames_gen));
        }
        output_samples_gen = src_data.output_frames_gen * channels;

        // TODO: Update ratio
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