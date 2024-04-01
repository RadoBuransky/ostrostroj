#include "graph.hpp"

SampleNode::SampleNode(Sample& sample, bool loop):
    sample(sample) {
}

SampleNode::~SampleNode() {    
    sample.unload();
}

bool SampleNode::pop(float& sample) {
    return true;
}
