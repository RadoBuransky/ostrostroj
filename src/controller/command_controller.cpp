#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "command_controller.hpp"
#include "midi_note.hpp"

static const std::array<MidiNote, 4> ROW2 = {MidiNote(E, 10), MidiNote(F, 10), MidiNote(F_, 10), MidiNote( G, 10)};
static const std::array<MidiNote, 4> ROW1 = {MidiNote( C, 10), MidiNote( C_, 10), MidiNote(D, 10), MidiNote(D_, 10)};

static const std::vector<MidiNote> SHUTDOWN_DEVICE_SEQ = {{ROW2.at(1), ROW1.at(2), ROW2.at(3)}};
static const std::vector<MidiNote> RESTART_DEVICE_SEQ  = {{ROW1.at(1), ROW2.at(2), ROW1.at(3)}};
static const std::vector<MidiNote> RESTART_SERVICE_SEQ = {{ROW1.at(1), ROW1.at(2), ROW1.at(3)}};
static const MidiNote PC_NEXT_NOTE = ROW2.at(0);
static const MidiNote PC_PREV_NOTE = ROW1.at(0);

CommandController::CommandController(uint8_t channel):
    commands({
        CommandSeq(RESTART_DEVICE_SEQ, Command::RESTART_DEVICE, channel),
        CommandSeq(SHUTDOWN_DEVICE_SEQ, Command::SHUTDOWN_DEVICE, channel),
        CommandSeq(RESTART_SERVICE_SEQ, Command::RESTART_SERVICE, channel)
    }) {
}

Command CommandController::note(uint8_t channel, uint8_t note, bool on, unsigned int clock, bool running) {
    SPDLOG_DEBUG("CMD   note[channel={},note={},on={}]", channel, note, on);
    if (on && note == PC_NEXT_NOTE.get_value()) {        
        SPDLOG_DEBUG("CMD   PC_NEXT");
        return Command::PC_NEXT;
    }
    if (on && note == PC_PREV_NOTE.get_value()) {
        SPDLOG_DEBUG("CMD   PC_PREV");
        return Command::PC_PREV;
    }
    for (CommandSeq& command : commands) {
        if (command.note(channel, note, on, clock, running)) {
            SPDLOG_DEBUG("CMD   command[command={}]", command.get_context());
            return (Command)command.get_context();
        }
    }
    SPDLOG_DEBUG("CMD   NOOP");
    return NOOP;
}