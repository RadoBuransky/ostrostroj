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

SampleNode::SampleNode(Sample& sample, bool loop):
    sample(sample) {
}

SampleNode::~SampleNode() {    
    sample.unload();
}

bool SampleNode::pop(float& sample) {
    return true;
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

void MuteNode::set_mute(bool mute) {
    muted = mute;
}

bool MuteNode::get_mute() const {
    return muted;
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