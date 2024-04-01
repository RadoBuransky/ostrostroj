#pragma once

#include <vector>
#include <thread>
#include "sample.hpp"

class Node {
    public:
        /**
         * Stateful operation. For stereo, first call returns left, second call right channel.
         * @returns `false` if this node is done and will never produce a sample.
        */
        virtual bool pop(float& sample) = 0;
};

class NoopNode : public Node {
    public:
        static NoopNode& getInstance() {
            static NoopNode instance;
            return instance;
        }
        NoopNode() {};
        NoopNode(NoopNode const &) = delete;
        void operator=(NoopNode const &) = delete;
        virtual bool pop(float& sample) {
            sample = 0.0;
            return true;
        }
};

class ChildNode : public Node {
    protected:
        Node& parent;
    public:
        ChildNode(Node& parent);
        Node& get_parent();
};

class DynamicNode : public Node {
    private:
        std::unique_ptr<Node> parent;
    public:
        DynamicNode();
        Node& get_parent();
        void set_parent(std::unique_ptr<Node> _parent);
        void reset_parent();
        virtual bool pop(float& sample);
};

class SampleNode : public Node {
    private:
        Sample& sample;
    public:
        SampleNode(Sample& sample, bool loop);
        virtual ~SampleNode();
        virtual bool pop(float& sample);
};

class MixingNode : public Node {
    private:
        std::vector<std::unique_ptr<Node>> nodes;
    public:
        void add_node(Node& node);
        virtual bool pop(float& sample);
};

class TrackNode : public ChildNode {
    private:
        bool started;
        bool muted;
    public:
        TrackNode(Node& parent);
        virtual bool pop(float& sample);
        void start();
        void stop();
        void set_mute(bool mute);
        bool get_mute() const;
};