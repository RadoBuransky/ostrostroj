#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "volmod_processor.hpp"
#include "midi_note.hpp"

static const std::vector<snd_seq_event_t> EMPTY_RESULT = {};

// Selection
static const std::array<MidiNote, VOLMOD_MOD_COUNT> MOD_ROW   = {MidiNote(G_, 9), MidiNote(A, 9), MidiNote(A_, 9), MidiNote(B, 9)};
static const std::array<MidiNote, VOLMOD_TRACK_COUNT> TRACK_ROW = {MidiNote(E , 9), MidiNote(F, 9), MidiNote(F_, 9), MidiNote(G, 9)};

// Input
static const uint8_t CHANNEL = 16;
static const uint8_t MW_MOD_CONTROL_CC = 83;
static const uint8_t BC_MOD_CONTROL_CC = 84;
static const uint8_t PB_MOD_CONTROL_CC = 85;
static const uint8_t AT_MOD_CONTROL_CC = 86;
static const std::array<uint8_t, VOLMOD_MOD_COUNT> MOD_CONTROL_CC = {
    MW_MOD_CONTROL_CC,
    BC_MOD_CONTROL_CC,
    PB_MOD_CONTROL_CC,
    AT_MOD_CONTROL_CC };
static const uint8_t BASE_VOLUME_CONTROL_CC = 87;
static const std::array<uint8_t, VOLMOD_TRACK_COUNT> VOLUME_CONTROL_CC = {
    BASE_VOLUME_CONTROL_CC,
    BASE_VOLUME_CONTROL_CC + 1,
    BASE_VOLUME_CONTROL_CC + 2,
    BASE_VOLUME_CONTROL_CC + 3};

// Output
static const uint8_t MW_CC = 1;
static const uint8_t BC_CC = 2;
static const uint8_t ELEKTRON_TRACK_LEVEL_CC = 95;
static const std::array<uint8_t, VOLMOD_TRACK_COUNT> ELEKTRON_TRACK_CHANNELS = {1, 2, 3, 4};

std::vector<uint8_t> VolModProcessor::getSelectedMods(uint8_t messageMod) {
    std::vector<uint8_t> selectedMods = {};
    for (uint8_t mod = 0; mod < VOLMOD_MOD_COUNT; mod++) {
        if (modSelected.at(mod)) {
            selectedMods.push_back(MOD_CONTROL_CC.at(mod));
        }
    }
    if (selectedMods.empty()) {
        selectedMods.push_back(MOD_CONTROL_CC.at(messageMod));
    }
    return selectedMods;
}

std::vector<uint8_t> VolModProcessor::getSelectedChannels(uint8_t messageMod) {
    std::vector<uint8_t> selectedTracks = {};
    for (uint8_t track = 0; track < VOLMOD_TRACK_COUNT; track++) {
        if (trackSelected.at(track)) {
            selectedTracks.push_back(ELEKTRON_TRACK_CHANNELS.at(track));
        }
    }
    if (selectedTracks.empty()) {
        selectedTracks.push_back(ELEKTRON_TRACK_CHANNELS.at(messageMod));
    }
    return selectedTracks;
}

snd_seq_event_t VolModProcessor::createEvent(uint8_t mod, uint8_t channel, int value) {
    snd_seq_event_t result = {};
    switch (mod) {
        case BASE_VOLUME_CONTROL_CC:
            result.type = SND_SEQ_EVENT_CONTROLLER;
            result.data.control.channel = channel;
            result.data.control.param = ELEKTRON_TRACK_LEVEL_CC;
            result.data.control.value = value;
            break;
        case MW_MOD_CONTROL_CC:
            result.type = SND_SEQ_EVENT_CONTROLLER;
            result.data.control.channel = channel; 
            result.data.control.param = MW_CC;
            result.data.control.value = value;
            break;
        case BC_MOD_CONTROL_CC:
            result.type = SND_SEQ_EVENT_CONTROLLER;
            result.data.control.channel = channel; 
            result.data.control.param = BC_CC;
            result.data.control.value = value;
            break;
        case AT_MOD_CONTROL_CC:
            result.type = SND_SEQ_EVENT_KEYPRESS;
            result.data.note.channel = channel;
            result.data.note.velocity = value;
            break;
        case PB_MOD_CONTROL_CC:
            result.type = SND_SEQ_EVENT_PITCHBEND;
            result.data.control.channel = channel; 
            result.data.control.value = (16384 * value / 127) - 8192;
            break;
        default:
            throw OstrostrojException(fmt::format("Unsupported mod![{}]", mod));
    }
    return result;
}

std::vector<snd_seq_event_t> VolModProcessor::selectedController(std::vector<uint8_t> selectedMods, std::vector<uint8_t> selectedChannels, int value) {
    std::vector<snd_seq_event_t> result = {};
    for (uint8_t mod = 0; mod < selectedMods.size(); mod++) {
        for (uint8_t channel = 0; channel < selectedChannels.size(); channel++) {
            snd_seq_event_t event = createEvent(selectedMods.at(mod), selectedChannels.at(channel), value);
            result.push_back(event);
        }
    }
    return result;
}

void VolModProcessor::note(snd_seq_ev_note_t noteEvent) {
    if (noteEvent.channel != CHANNEL) {
        return;
    }
    for (uint8_t mod = 0; mod < MOD_ROW.size(); mod++) {
        if (noteEvent.note == MOD_ROW.at(mod).get_value()) {
            modSelected.at(mod) = !modSelected.at(mod);
            return;
        }
    }
    for (uint8_t track = 0; track < TRACK_ROW.size(); track++) {
        if (noteEvent.note == TRACK_ROW.at(track).get_value()) {
            trackSelected.at(track) = !trackSelected.at(track);
            return;
        }
    }
}

std::vector<snd_seq_event_t> VolModProcessor::controller(snd_seq_ev_ctrl_t controllerEvent) {
    std::vector<snd_seq_event_t> result;
    if (controllerEvent.channel != CHANNEL) {
        return EMPTY_RESULT;
    }
    for (uint8_t i = 0; i < VOLMOD_TRACK_COUNT; i++) {
        if (controllerEvent.param == VOLUME_CONTROL_CC.at(i)) {
            result.push_back(createEvent(BASE_VOLUME_CONTROL_CC, ELEKTRON_TRACK_CHANNELS.at(i), controllerEvent.value));
            return result;
        }
    }
    for (uint8_t mod = 0; mod < VOLMOD_MOD_COUNT - 1; mod++) {
        if (controllerEvent.param == MOD_CONTROL_CC.at(mod)) {
            std::vector<uint8_t> selectedMods = getSelectedMods(mod);
            std::vector<uint8_t> selectedChannels = getSelectedChannels(mod);
            return selectedController(selectedMods, selectedChannels, controllerEvent.value);
        }
    }
    return EMPTY_RESULT;
}

VolModProcessor::VolModProcessor():
    trackSelected() {    
}

std::vector<snd_seq_event_t> VolModProcessor::process(snd_seq_event_t &event) {
    switch (event.type) {
        case SND_SEQ_EVENT_NOTEON:
        case SND_SEQ_EVENT_NOTEOFF:
            note(event.data.note);
            break;
        case SND_SEQ_EVENT_CONTROLLER:
            return controller(event.data.control);
    }
    return EMPTY_RESULT;
}

std::array<bool, VOLMOD_TRACK_COUNT>& VolModProcessor::getTrackSelected() {
    return trackSelected;
}

std::array<bool, VOLMOD_MOD_COUNT>& VolModProcessor::getModSelected() {
    return modSelected;
}