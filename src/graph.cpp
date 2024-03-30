#include "graph.hpp"

ChildNode::ChildNode(Node& _parent):
    parent(_parent) {
}

Node& ChildNode::get_parent() {
    return parent;
}

MuteNode::MuteNode(Node& parent):
    ChildNode(parent),
    muted(false) {
}

bool MuteNode::pop(float& sample) {
    if (muted) {
        float other_sample;
        return parent.pop(other_sample);
    }
    return parent.pop(sample);
}

TransportNode::TransportNode(Node& parent):
    ChildNode(parent),
    started(false) {
}

bool TransportNode::pop(float& sample) {
    if (started) {
        return parent.pop(sample);
    }
    sample = 0.0;
    return true;
}

void TransportNode::start() {
    started = true;
}

void TransportNode::stop() {
    started = false;
}