#pragma once

#include "clipblock.hpp"

class ClipFx {
    private:
        // 2.5ms
        static constexpr sf_count_t xfade_half_length = 240;
        ClipFx() = default;

        std::array<float, 2*xfade_half_length> xfade_loop_init_buffer(ClipBlock& loop_head);
        void xfade_loop_copy_result(ClipBlock& loop_head, std::array<float, 2*xfade_half_length>& buffer);
    public:
        static ClipFx& get() {
            static ClipFx instance;
            return instance;
        }
        ClipFx(ClipFx const&) = delete;
        void operator=(ClipFx const&) = delete;

        void xfade_loop(ClipBlock& loop_head);
};