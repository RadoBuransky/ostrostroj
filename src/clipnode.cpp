#include <spdlog/spdlog.h>
#include "graph.hpp"

ClipNode::ClipNode(Clip& _clip, bool _loop):
    clip(_clip),
    block(_clip.get_head()),
    loop(_loop),
    it_next(_clip.get_head().get_buffer().cbegin()),
    it_end(_clip.get_head().get_buffer().cend()) {
    spdlog::trace(std::format("{} (this=0x{:x})", __FUNCTION__, reinterpret_cast<intptr_t>(this)));
}

ClipNode::~ClipNode() {    
    clip.unload();
    spdlog::trace("~ClipNode"); 
}

bool ClipNode::pop(float& sample) {
    if (it_next != it_end) {
        sample = *it_next;
        it_next++;
        spdlog::debug(std::format("ClipNode sample popped. [{:g}, this=0x{:x} it_next=0x{:x}]", sample, reinterpret_cast<intptr_t>(this),
            reinterpret_cast<intptr_t>(&(*it_next))));
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
    it_next = block.get().get_buffer().cbegin();
    it_end = block.get().get_buffer().cend();
    if (it_next == it_end) {
        spdlog::warn("ClipNode empty buffer!");
        return false;
    }
    return pop(sample);
}

bool ClipNode::load_next() {
    return clip.load_next();
}
