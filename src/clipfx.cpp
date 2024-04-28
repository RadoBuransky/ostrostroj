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
    for (sf_count_t i = 0; i < buffer_size; i++) {
        if (i < window_half_size || i >= buffer_size - window_half_size) {
            result[i] = buffer[i];
        } else {
            float window_value = 0.0;
            float window_weight = 0.0;
            for (sf_count_t j = -window_half_size; j < window_half_size + 1; j++) {
                float weight = 1.0 - std::abs(static_cast<float>(j) / static_cast<float>(window_half_size + 1));
                window_value += buffer[i+j] * weight;
                window_weight += weight;
            }
            if (window_weight == 0.0) {
                result[i] = buffer[i];
            } else {
                result[i] = window_value / window_weight;
            }
        }
    }
    xfade_loop_copy_result(loop_head, result);
}