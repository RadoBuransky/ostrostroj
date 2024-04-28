#include "common.hpp"
#include "clipfx.hpp"

std::array<float, 2*ClipFx::xfade_half_length> ClipFx::xfade_loop_init_buffer(ClipBlock& loop_head) {
    std::array<float, 2*xfade_half_length> result;
    const sf_count_t total_frames = loop_head.get_total_frames();
    std::reference_wrapper<ClipBlock> block = loop_head;
    sf_count_t clip_pos = 0;
    sf_count_t result_pos = 0;
    bool has_next = true;
    while (has_next) {
        clip_buffer& block_buffer = block.get().get_buffer();
        const sf_count_t buffer_size = block_buffer.size();
        for (sf_count_t block_pos = 0; block_pos < buffer_size; block_pos++) {
            if ((clip_pos < xfade_half_length) || (clip_pos >= (total_frames - xfade_half_length))) {
                result.at(result_pos) = block_buffer[block_pos];
                result_pos++;
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

void ClipFx::xfade_loop_copy_result(ClipBlock& loop_head, std::array<float, 2*xfade_half_length>& result) {    
    const sf_count_t total_frames = loop_head.get_total_frames();
    std::reference_wrapper<ClipBlock> block = loop_head;
    sf_count_t clip_pos = 0;
    sf_count_t result_pos = 0;
    bool has_next = true;
    while (has_next) {
        clip_buffer& block_buffer = block.get().get_buffer();
        const sf_count_t buffer_size = block_buffer.size();
        for (sf_count_t block_pos = 0; block_pos < buffer_size; block_pos++) {
            if ((clip_pos < xfade_half_length) || (clip_pos >= (total_frames - xfade_half_length))) {
                block_buffer[block_pos] = result.at(result_pos);
                result_pos++;
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
    if (loop_head.get_buffer().size() < xfade_half_length) {
        SPDLOG_WARN("Loop is too short for a nice xfade!");
        return;
    }
    std::array<float, 2*xfade_half_length> buffer = xfade_loop_init_buffer(loop_head);
    std::array<float, 2*xfade_half_length> result;    
    constexpr sf_count_t buffer_size = buffer.size();
    for (sf_count_t i = 0; i < buffer_size; i++) {
        float window_value = 0.0;
        float window_weight = 0.0;
        sf_count_t window_size = std::min(i, (buffer_size - i) - 1);
        for (sf_count_t j = -window_size; j < window_size + 1; j++) {
            float weight = 1.0 - std::abs(j / (window_size + 1));
            window_value += buffer[i+j] * weight;
            window_weight += weight;
        }
        result[i] = window_value / window_weight;
    }
    xfade_loop_copy_result(loop_head, result);
}