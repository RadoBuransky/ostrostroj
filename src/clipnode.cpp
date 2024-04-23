#include <spdlog/spdlog.h>
#include "graph.hpp"

void ClipNode::update_pointers(ClipBlock& _block) {
    current_frame = _block.get_buffer().data();
    end_frame = current_frame + _block.get_buffer_frames(); // TODO: Channels?
}

ClipNode::ClipNode(Clip& _clip, bool _loop):
    clip(_clip),
    block(_clip.get_head()),
    loop(_loop) {
    update_pointers(block.load().get());
}

bool ClipNode::pop(float& sample) {
    if (current_frame < end_frame) {
        sample = *current_frame;
        current_frame++;
        position++;
        return true;        
    }
    if (block.load().get().has_next()) {
        block = std::ref(block.load().get().get_next());
        if (block.load().get().get_start_pos() != position) {
            SPDLOG_WARN(std::format("Unexpected block position! [block={},expected={}]", block.load().get().get_start_pos(), position.load()));
        }
    } else {
        if (!loop) {
            return false;
        }
        SPDLOG_DEBUG(std::format("Lopp restart. [path={}, this=0x{:x}, clip=0x{:x}, current_frame=0x{:x}, end_frame=0x{:x}, block=0x{:x}]",
            clip.get_path().c_str(), reinterpret_cast<intptr_t>(this), reinterpret_cast<intptr_t>(&clip),
            reinterpret_cast<intptr_t>(current_frame.load()), reinterpret_cast<intptr_t>(end_frame.load()),
            reinterpret_cast<intptr_t>(&block.load().get())));
        block = std::ref(clip.get_head());
        position = 0;
    }
    update_pointers(block.load().get());
    return pop(sample);
}

Clip& ClipNode::get_clip() const {
    return clip;
}
