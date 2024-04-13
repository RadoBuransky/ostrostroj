#include <spdlog/spdlog.h>
#include "graph.hpp"

void ClipNode::update_pointers(ClipBlock& _block) {
    current_frame = _block.get_buffer().data();
    end_frame = _block.get_buffer().data() + _block.get_buffer_frames();
}

ClipNode::ClipNode(Clip& _clip, bool _loop):
    clip(_clip),
    block(_clip.get_head()),
    loop(_loop) {
    update_pointers(block.get());
    spdlog::trace(std::format("{} (this=0x{:x})", __FUNCTION__, reinterpret_cast<intptr_t>(this)));
}

ClipNode::~ClipNode() {    
    clip.unload();
    spdlog::trace("~ClipNode"); 
}

bool ClipNode::pop(float& sample) {
    if (current_frame < end_frame) {
        sample = *current_frame;
        current_frame++;
        spdlog::debug(std::format("ClipNode sample popped. [{:g}, this=0x{:x} it_next=0x{:x}]", sample, reinterpret_cast<intptr_t>(this),
            reinterpret_cast<intptr_t>(&(*current_frame))));
        return true;        
    }
    if (block.get().has_next()) {
        block = std::ref(block.get().get_next());
    } else {
        if (loop) {
            spdlog::debug("Next loop.");
            block = std::ref(clip.get_head());
        } else {
            spdlog::debug("ClipNode done.");
            return false;
        }
    }
    update_pointers(block.get());
    return pop(sample);
}

bool ClipNode::load_next() {
    return clip.load_next();
}
