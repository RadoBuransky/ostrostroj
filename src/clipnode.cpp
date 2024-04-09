#include "graph.hpp"

ClipNode::ClipNode(Clip& clip, bool loop):
    clip(clip),
    block(clip.get_head()),
    loop(loop),
    it_next(clip.get_head().get_buffer().cbegin()),
    it_end(clip.get_head().get_buffer().cend()) {
}

ClipNode::~ClipNode() {    
    clip.unload();
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
