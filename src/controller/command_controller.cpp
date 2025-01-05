#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "command_controller.hpp"
#include "midi_note.hpp"

static const std::vector<MidiNote> RESTART_DEVICE_SEQ = {{MidiNote(F, 4), MidiNote(D, 5), MidiNote(G, 4)}};
static const std::vector<MidiNote> SHUTDOWN_DEVICE_SEQ = {{MidiNote(C_, 5), MidiNote(F_, 4), MidiNote(D_, 5)}};
static const std::vector<MidiNote> RESTART_SERVICE_SEQ = {{MidiNote(F, 4), MidiNote(F_, 4), MidiNote(G, 4)}};
static const std::vector<MidiNote> PC_NEXT_SEQ = {{MidiNote(C, 4)}};
static const std::vector<MidiNote> PC_PREV_SEQ = {{MidiNote(C_, 4)}};
static const std::vector<MidiNote> PC_DONE_SEQ = {{MidiNote(D, 4)}};

CommandController::CommandController(uint8_t channel):
    commands({
        CommandSeq(RESTART_DEVICE_SEQ, Command::RESTART_DEVICE, channel),
        CommandSeq(SHUTDOWN_DEVICE_SEQ, Command::SHUTDOWN_DEVICE, channel),
        CommandSeq(RESTART_SERVICE_SEQ, Command::RESTART_SERVICE, channel),
        CommandSeq(PC_NEXT_SEQ, Command::PC_NEXT, channel),
        CommandSeq(PC_PREV_SEQ, Command::PC_PREV, channel),
        CommandSeq(PC_DONE_SEQ, Command::PC_DONE, channel)
    }) {
}

Command CommandController::note(uint8_t channel, uint8_t note, bool on, unsigned int clock, bool running) {
    for (CommandSeq& command : commands) {
        if (command.note(channel, note, on, clock, running)) {
            return (Command)command.get_context();
        }
    }
    return NOOP;
}