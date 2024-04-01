#include "graph.hpp"

ChildNode::ChildNode(Node& _parent):
    parent(_parent) {
}

Node& ChildNode::get_parent() {
    return parent;
}

DynamicNode::DynamicNode():
    parent(std::make_unique<NoopNode>()) {    
}

Node& DynamicNode::get_parent() {
    return *parent;
}

void DynamicNode::set_parent(std::unique_ptr<Node> _parent) {    
    parent = std::move(_parent);
}

void DynamicNode::reset_parent() {
    parent = std::make_unique<NoopNode>();
}

bool DynamicNode::pop(float& sample) {
    return parent->pop(sample);
}

TrackNode::TrackNode(Node& parent):
    ChildNode(parent),
    started(false),
    muted(false) {
}

bool TrackNode::pop(float& sample) {
    if (started) {
        if (muted) {
            float other_sample;
            return parent.pop(other_sample);
        }
        return parent.pop(sample);
    }
    sample = 0.0;
    return true;
}

void TrackNode::start() {
    started = true;
}

void TrackNode::stop() {
    started = false;
}

void TrackNode::set_mute(bool mute) {
    muted = mute;
}

bool TrackNode::get_mute() const {
    return muted;
}