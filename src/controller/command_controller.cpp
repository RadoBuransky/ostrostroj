#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "command_controller.hpp"
#include "midi_note.hpp"

static const std::vector<MidiNote> RESTART_DEVICE_SEQ = {{MidiNote(F, 4), MidiNote(D, 5), MidiNote(G, 4)}};
static const std::vector<MidiNote> SHUTDOWN_DEVICE_SEQ = {{MidiNote(C_, 5), MidiNote(F_, 4), MidiNote(D_, 5)}};
static const std::vector<MidiNote> RESTART_SERVICE_SEQ = {{MidiNote(F, 4), MidiNote(F_, 4), MidiNote(G, 4)}};
static const MidiNote PC_PREV_NOTE = MidiNote(C, 4);
static const MidiNote PC_NEXT_NOTE = MidiNote(C_, 4);

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