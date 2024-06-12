#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "command_controller.hpp"
#include "midi_note.hpp"

static const std::vector<MidiNote> RESTART_SEQ = {{MidiNote(F, 4), MidiNote(D, 5), MidiNote(G, 4)}};
static const std::vector<MidiNote> SHUTDOWN_SEQ = {{MidiNote(C_, 5), MidiNote(F_, 4), MidiNote(D_, 5)}};

CommandController::CommandController():
    commands({
        CommandSeq(RESTART_SEQ, Command::RESTART),
        CommandSeq(SHUTDOWN_SEQ, Command::SHUTDOWN)
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