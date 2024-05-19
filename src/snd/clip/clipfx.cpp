#include "common.hpp"
#include "clipfx.hpp"

std::array<float, 2*ClipFx::xfade_half_size> ClipFx::xfade_loop_init_buffer(ClipBlock& loop_head) {
    std::array<float, 2*xfade_half_size> result;
    const sf_count_t total_frames = loop_head.get_total_frames();
    std::reference_wrapper<ClipBlock> block = loop_head;
    sf_count_t clip_pos = 0;
    bool has_next = true;
    while (has_next) {
        clip_buffer& block_buffer = block.get().get_buffer();
        const sf_count_t buffer_size = block.get().get_buffer_frames();
        for (sf_count_t block_pos = 0; block_pos < buffer_size; block_pos++) {
            if (clip_pos < xfade_half_size) {
                result.at(xfade_half_size+clip_pos) = block_buffer[block_pos];
            } else {
                if (clip_pos >= (total_frames - xfade_half_size)) {
                    result.at(clip_pos - (total_frames - xfade_half_size)) = block_buffer[block_pos];
                }
            }
            clip_pos++;
        }
        has_next = block.get().has_next();
        if (has_next) {
            block = block.get().get_next();
        }
    }    
    return result;
}

void ClipFx::xfade_loop_copy_result(ClipBlock& loop_head, std::array<float, 2*xfade_half_size>& result) {    
    const sf_count_t total_frames = loop_head.get_total_frames();
    std::reference_wrapper<ClipBlock> block = loop_head;
    sf_count_t clip_pos = 0;
    bool has_next = true;
    while (has_next) {
        clip_buffer& block_buffer = block.get().get_buffer();
        const sf_count_t block_buffer_size = block.get().get_buffer_frames();
        for (sf_count_t block_pos = 0; block_pos < block_buffer_size; block_pos++) {
            if (clip_pos < xfade_half_size) {
                block_buffer[block_pos] = result.at(xfade_half_size+clip_pos);
            } else {
                if (clip_pos >= (total_frames - xfade_half_size)) {
                    block_buffer[block_pos] = result.at(clip_pos - (total_frames - xfade_half_size));
                }
            }
            clip_pos++;
        }
        has_next = block.get().has_next();
        if (has_next) {
            block = block.get().get_next();
        }
    }
}

void ClipFx::xfade_loop(ClipBlock& loop_head) {
    if (loop_head.get_buffer_frames() < xfade_half_size) {
        SPDLOG_WARN("Loop is too short for a nice xfade!");
        return;
    }
    std::array<float, 2*xfade_half_size> buffer = xfade_loop_init_buffer(loop_head);
    std::array<float, 2*xfade_half_size> result;    
    constexpr sf_count_t buffer_size = buffer.size();

    SPDLOG_WARN("====INIT====");
    for (sf_count_t i = 0; i < buffer_size; i++) {
        SPDLOG_WARN("{:3} {:.6f}", i, buffer[i]);
    }

    constexpr float window_half_size = xfade_half_size/20;
    float start_sum = 0.0;
    float end_sum = 0.0;
    sf_count_t start_buffer_pos = xfade_half_size + window_half_size - 1;
    sf_count_t end_buffer_pos = xfade_half_size - window_half_size;
    for (sf_count_t i = 0; i < window_half_size; i++) {
        start_sum += buffer[start_buffer_pos];
        buffer[start_buffer_pos] = start_sum/(float)(i + 1);
        start_buffer_pos--;
        
        end_sum += buffer[end_buffer_pos];
        buffer[end_buffer_pos] = end_sum/(float)(i + 1);
        end_buffer_pos++;
    }

    SPDLOG_WARN("====RESULT====");
    for (sf_count_t i = 0; i < buffer_size; i++) {
        float start_sample;
        float end_sample;
        if (i < xfade_half_size) {
            start_sample = buffer[(buffer_size - i) - 1];
            end_sample = buffer[i];
        } else {
            start_sample = buffer[i];
            end_sample = buffer[(buffer_size - i) - 1];
        }
        //float ni = ((float)i / (float)(xfade_half_size)) - 1.0;
        //result[i] = start_sample*std::sqrt((1.0 + ni)/2.0) + end_sample*std::sqrt((1.0 - ni)/2.0);
        result[i] = start_sample*((float)i/(float)buffer_size) + end_sample*((float)(buffer_size-i)/(float)buffer_size);

        SPDLOG_WARN("{:3} {:.6f}->{:.6f}", i, buffer[i], result[i]);
    }
    xfade_loop_copy_result(loop_head, result);
}