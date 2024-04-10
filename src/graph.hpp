#pragma once

#include <vector>
#include <thread>
#include "clip.hpp"

class Node {
    public:
        Node();
        virtual ~Node();
        /**
         * Stateful operation. For stereo, first call returns left, second call right channel.
         * @returns `false` if this node is done and will never produce a sample.
        */
        virtual bool pop(float& sample) = 0;
};

class NoopNode : public Node {
    public:
        NoopNode();
        virtual ~NoopNode();
        virtual bool pop(float& sample);
};

class ChildNode : public Node {
    protected:
        Node& parent;
    public:
        ChildNode(Node& parent);
        virtual ~ChildNode();
        Node& get_parent() const;
};

class DynamicNode : public Node {
    private:
        std::unique_ptr<Node> parent;
    public:
        DynamicNode();
        virtual ~DynamicNode();
        DynamicNode(DynamicNode&) = delete;
        DynamicNode& operator=(DynamicNode&) = delete;
        DynamicNode(DynamicNode&&) = delete;
        DynamicNode& operator=(DynamicNode&&) = delete;
        Node& get_parent() const;
        void set_parent(std::unique_ptr<Node>&& _parent);
        void reset_parent();
        virtual bool pop(float& sample);
};

class ClipNode : public Node {
    private:
        Clip& clip;
        std::reference_wrapper<ClipBlock> block;
        bool loop;
        std::vector<float>::const_iterator it_next;
        std::vector<float>::const_iterator it_end;
    public:
        ClipNode(Clip& clip, bool loop);
        virtual ~ClipNode();
        virtual bool pop(float& sample);
        bool load_next();
};

class MixingNode : public Node {
    private:
        std::vector<std::unique_ptr<Node>> nodes;
    public:
        virtual ~MixingNode();
        void add_node(Node& node);
        virtual bool pop(float& sample);
        std::vector<std::reference_wrapper<Node>> get_parents() const;
};

class TrackNode : public ChildNode {
    private:
        bool started;
        bool muted;
    public:
        TrackNode(Node& parent);
        virtual ~TrackNode();
        virtual bool pop(float& sample);
        void start();
        void stop();
        void set_mute(bool mute);
        bool get_mute() const;
};