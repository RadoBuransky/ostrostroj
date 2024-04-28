#pragma once

#include "clipblock.hpp"

class ClipFx {
    private:
        static constexpr sf_count_t window_half_size = 100;
        static constexpr sf_count_t xfade_half_size = 2*window_half_size;
        ClipFx() = default;

        std::array<float, 2*xfade_half_size> xfade_loop_init_buffer(ClipBlock& loop_head);
        void xfade_loop_copy_result(ClipBlock& loop_head, std::array<float, 2*xfade_half_size>& buffer);
    public:
        static ClipFx& get() {
            static ClipFx instance;
            return instance;
        }
        ClipFx(ClipFx const&) = delete;
        void operator=(ClipFx const&) = delete;

        void xfade_loop(ClipBlock& loop_head);
};