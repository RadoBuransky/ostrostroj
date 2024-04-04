#include "graph.hpp"

ClipNode::ClipNode(Clip& clip, bool loop):
    clip(clip),
    position(0) {
}

ClipNode::~ClipNode() {    
    clip.unload();
}

bool ClipNode::pop(float& sample) {
    position++;
    // TODO: ...
    return true;
}

bool ClipNode::load_next() {
    return clip.load_next();
}
