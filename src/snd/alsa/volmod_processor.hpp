#pragma once

static const uint8_t VOLMOD_TRACK_COUNT = 4;
static const uint8_t VOLMOD_MOD_COUNT = 4;

class VolModProcessor {
    private:
        std::array<bool, VOLMOD_TRACK_COUNT> trackSelected;
        std::array<bool, VOLMOD_MOD_COUNT> modSelected;
        std::vector<uint8_t> getSelectedMods(uint8_t messageMod);
        std::vector<uint8_t> getSelectedChannels(uint8_t messageMod);
        snd_seq_event_t createEvent(uint8_t mod, uint8_t channel, int value);
        std::vector<snd_seq_event_t> selectedController(std::vector<uint8_t> selectedMods, std::vector<uint8_t> selectedTracks, int value);
        void note(snd_seq_ev_note_t noteEvent);
        std::vector<snd_seq_event_t> controller(snd_seq_ev_ctrl_t controllerEvent);
    public:
        VolModProcessor();
        virtual ~VolModProcessor() = default;
        std::vector<snd_seq_event_t> process(snd_seq_event_t &event);
        std::array<bool, VOLMOD_TRACK_COUNT>& getTrackSelected();
        std::array<bool, VOLMOD_MOD_COUNT>& getModSelected();
};