#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "program_change.hpp"
#include "session.hpp"

ProgramChange::ProgramChange(Engine& _engine):
    engine(_engine),
    selected_song(engine.session.get_song()),
    selected_pattern(engine.session.get_pattern()) {
}

void ProgramChange::on_program_changed() {
    selected_song = engine.session.get_song();
    selected_pattern = engine.session.get_pattern();
}

void ProgramChange::on_change_selection(int delta) {    
    // TODO:
}

void ProgramChange::on_fader(float mix) {    
    // TODO:
}