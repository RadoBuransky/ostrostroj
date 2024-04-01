#include "graph.hpp"

ClipNode::ClipNode(Clip& clip, bool loop):
    clip(clip) {
}

ClipNode::~ClipNode() {    
    clip.unload();
}

bool ClipNode::pop(float& sample) {
    return true;
}
