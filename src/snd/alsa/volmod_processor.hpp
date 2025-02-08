#pragma once

class VolModProcessor {
    public:
        VolModProcessor();
        virtual ~VolModProcessor() = default;
        std::vector<snd_seq_event_t> process(snd_seq_event_t &event);
};