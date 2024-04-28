#include "testclip.hpp"
#include "clipfx.hpp"

RampDownClip::RampDownClip():
    head(blocks) {
  init(head.get_buffer_frames() * blocks, 0, head);
  ClipFx::get().xfade_loop(head);
}

void RampDownClip::init(float total_frames, int pos, ClipBlock& block) {
    clip_buffer& buffer = block.get_buffer();
    for (sf_count_t i = 0; i < static_cast<sf_count_t>(buffer.size()); i++) {
        buffer[i] = std::max(1.0 - ((pos + i) * 2.0 / total_frames), -1.0);
    }
    if (block.has_next()) {
        init(total_frames, pos + buffer.size(), block.get_next());
    }
}

ClipBlock& RampDownClip::get_head() {
    return head;
}