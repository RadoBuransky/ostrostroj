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

class ChildNode : public Node {
    protected:
        Node& parent;
    public:
        ChildNode(Node& parent);
        Node& get_parent();
};

class NoopNode : public Node {
    private:
        NoopNode() {};
    public:
        static NoopNode& getInstance() {
            static NoopNode instance;
            return instance;
        }
        NoopNode(NoopNode const &) = delete;
        void operator=(NoopNode const &) = delete;
        virtual bool pop(float& sample) {
            sample = 0.0;
            return true;
        }
};

class SampleNode : public Node {
    private:
        SampleReader sampleReader;
    public:
        SampleNode(const Sample& sample, bool loop): sampleReader(SampleReader(sample, loop)) {};
        virtual bool pop(float& sample) {return false;};
};

class MixingNode : public Node {
    private:
        std::vector<std::unique_ptr<Node>> nodes;
    public:
        void add_node(Node& node);
        virtual bool pop(float& sample);
};

class MuteNode : public ChildNode {
    private:
        bool muted;
    public:
        MuteNode(Node& parent);
        virtual bool pop(float& sample);
        void set_mute(bool mute);
        bool get_mute() const;
};

class TransportNode : public ChildNode {
    private:
        bool started;
    public:
        TransportNode(Node& parent);
        virtual bool pop(float& sample);
        void start();
        void stop();
};