#include <spdlog/spdlog.h>
#include "graph.hpp"

Node::Node() {
    spdlog::debug(std::format("{} (this=0x{:x})", __FUNCTION__, reinterpret_cast<intptr_t>(this)));
}

Node::~Node() {
    spdlog::debug(std::format("{} (this=0x{:x})", __FUNCTION__, reinterpret_cast<intptr_t>(this)));
}

NoopNode::NoopNode() {
    spdlog::debug(std::format("{} (this=0x{:x})", __FUNCTION__, reinterpret_cast<intptr_t>(this)));
}

NoopNode::~NoopNode() {
    spdlog::debug(std::format("{} (this=0x{:x})", __FUNCTION__, reinterpret_cast<intptr_t>(this)));
}

bool NoopNode::pop(float& sample) {
    sample = 0.0;
    return true;
}

ChildNode::ChildNode(Node& _parent):
    parent(_parent) {
    spdlog::debug(std::format("{} (this=0x{:x})", __FUNCTION__, reinterpret_cast<intptr_t>(this)));
}

ChildNode::~ChildNode() {
    spdlog::debug("~ChildNode"); 
}

Node& ChildNode::get_parent() const {
    return parent;
}

DynamicNode::DynamicNode():
    parent(std::make_unique<NoopNode>()) {
    spdlog::debug(std::format("{} (this=0x{:x}, parent=0x{:x})", __FUNCTION__, reinterpret_cast<intptr_t>(this), reinterpret_cast<intptr_t>(parent.get())));
}

DynamicNode::~DynamicNode() {
    spdlog::debug("~DynamicNode"); 
}

Node& DynamicNode::get_parent() const {
    return *parent;
}

void DynamicNode::set_parent(std::unique_ptr<Node>&& _parent) {    
    const Node* before = parent.get();
    spdlog::debug(std::format("{} 1 (this=0x{:x}, before=0x{:x}, _parent=0x{:x})",
    __FUNCTION__, reinterpret_cast<intptr_t>(this), reinterpret_cast<intptr_t>(before), reinterpret_cast<intptr_t>(_parent.get())));
    float sample;
    pop(sample);
    parent.reset();
    parent = std::move(_parent);
    spdlog::debug(std::format("{} 2 (this=0x{:x}, before=0x{:x}, after=0x{:x})",
    __FUNCTION__, reinterpret_cast<intptr_t>(this), reinterpret_cast<intptr_t>(before), reinterpret_cast<intptr_t>(parent.get())));
}

void DynamicNode::reset_parent() {
    const Node* before = parent.get();
    parent = std::make_unique<NoopNode>();
    spdlog::debug(std::format("{} (this=0x{:x}, before=0x{:x}, after=0x{:x})",
    __FUNCTION__, reinterpret_cast<intptr_t>(this), reinterpret_cast<intptr_t>(before), reinterpret_cast<intptr_t>(parent.get())));
}

bool DynamicNode::pop(float& sample) {
    return parent->pop(sample);
}

MixingNode::~MixingNode() {
    spdlog::debug("~MixingNode"); 
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
    started(false),
    muted(false) {
}

TrackNode::~TrackNode() {
    spdlog::debug("~TrackNode"); 
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