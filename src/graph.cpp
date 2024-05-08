#include "common.hpp"
#include "graph.hpp"

bool NoopNode::pop(float&) {
    return false;
}

ChildNode::ChildNode(Node& _parent):
    parent(_parent) {
}

Node& ChildNode::get_parent() const {
    return parent;
}

DynamicNode::DynamicNode():
    parent(std::make_unique<NoopNode>()) {
}

Node& DynamicNode::get_parent() const {
    return *parent;
}

void DynamicNode::set_parent(std::unique_ptr<Node>&& _parent) {
    parent = std::move(_parent);
}

bool DynamicNode::reset_parent() {
    if (dynamic_cast<NoopNode*>(parent.get())) {
        return false;
    }
    parent = std::make_unique<NoopNode>();
    return true;
}

bool DynamicNode::pop(float& sample) {
    return parent->pop(sample);
}

bool MixingNode::pop(float& sample) {
    float node_sample;
    sample = 0.0;
    std::vector<std::unique_ptr<Node>>::iterator it = nodes.begin();
    while (it != nodes.end()) {
        if (it->get()->pop(node_sample)) {
            sample += node_sample;
            it++;
        } else {
            it = nodes.erase(it);
        }
    }
    return !nodes.empty();
}

std::vector<std::reference_wrapper<Node>> MixingNode::get_parents() const {
    std::vector<std::reference_wrapper<Node>> result = {};
    for (const std::unique_ptr<Node>& node : nodes) {
        result.push_back(*node.get());
    }
    return result;
}

TrackNode::TrackNode(Node& _parent):
    ChildNode(_parent),
    muted(false) {
}

bool TrackNode::pop(float& sample) {
    if (muted) {
        float ignored_sample;
        return parent.pop(ignored_sample);
    }
    return parent.pop(sample);
}

void TrackNode::set_mute(bool mute) {
    muted = mute;
}

bool TrackNode::get_mute() const {
    return muted;
}