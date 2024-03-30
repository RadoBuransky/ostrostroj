#include "graph.hpp"

ChildNode::ChildNode(std::unique_ptr<Node> _parent):
    parent(std::move(_parent)) {    
}

Node& ChildNode::get_parent() {
    return *parent.get();
}

void ChildNode::set_parent(std::unique_ptr<Node> _parent) {
    parent = std::move(_parent);
}

TransportNode::TransportNode(std::unique_ptr<Node> _parent):
    ChildNode(std::move(_parent)),
    started(false) {
}

void TransportNode::start() {
    started = true;
}

void TransportNode::stop() {
    started = false;
}