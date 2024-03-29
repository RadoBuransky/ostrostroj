#pragma once

#include "sample.hpp"

class Node {
    public:
        /**
         * @returns `false` if this node is done and will never produce a sample.
        */
        virtual bool pop(int channel, float& sample) = 0;
};

// TODO: Can this be actually the SampleReader?
class SampleNode : public Node {
    private:
        SampleReader sampleReader;
    public:
        SampleNode(const Sample& sample);
        virtual bool pop(int channel, float& sample);
};

class MixingNode : public Node {
    private:
        std::vector<std::unique_ptr<Node>> nodes;
    public:
        void add_node(Node& node);
        // TODO: Remove node when done
        virtual bool pop(int channel, float& sample);
};

class MuteNode : public Node {
    private:
        Node& src;
        bool mute;
    public:
        MuteNode(Node& src);
        void set_mute(bool mute);
        bool get_mute() const;
};

// TODO: graph is dynamic, nodes can be added and removed (play one shot, change program)