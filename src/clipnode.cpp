#include <spdlog/spdlog.h>
#include "graph.hpp"

ClipNode::ClipNode(Clip& _clip, bool _loop):
    clip(_clip),
    block(_clip.get_head()),
    loop(_loop),
    it_next(_clip.get_head().get_buffer().cbegin()),
    it_end(_clip.get_head().get_buffer().cend()) {
    spdlog::debug(std::format("{} (this=0x{:x})", __FUNCTION__, reinterpret_cast<intptr_t>(this)));
}

ClipNode::~ClipNode() {    
    clip.unload();
    spdlog::debug("~ClipNode"); 
}

bool ClipNode::pop(float& sample) {
    if (it_next != it_end) {
        sample = *it_next;
        it_next++;
        return true;        
    }
    if (block.get().has_next()) {
        block = std::ref(block.get().get_next());
    } else {
        if (loop) {
            block = std::ref(clip.get_head());
        } else {
            return false;
        }
    }
    it_next = block.get().get_buffer().cbegin();
    it_end = block.get().get_buffer().cend();
    return true;
}

bool ClipNode::load_next() {
    return clip.load_next();
}
