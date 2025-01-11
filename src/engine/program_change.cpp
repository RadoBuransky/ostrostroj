#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "program_change.hpp"
#include "session.hpp"
#include "engine.hpp"

ProgramChange::ProgramChange(Engine& _engine):
    engine(_engine),
    selected_song_index(0),
    selected_pattern_index(0) {
    on_program_changed();
}

void ProgramChange::update_display() {    
    MainScreen& main_screen = engine.display.get_main_screen();
    Project& project = engine.session.get_project();
    Song& selected_song = project.get_songs().at(selected_song_index);
    main_screen.set_song_count(project.get_songs().size());
    main_screen.set_pattern_count(selected_song.get_patterns().size());
    main_screen.set_active_song_index(engine.session.get_song().get_number() - 1);
    main_screen.set_selected_song_index(selected_song_index);
    main_screen.set_active_pattern_index(engine.session.get_pattern().get_number() - 1);
    main_screen.set_selected_pattern_index(selected_pattern_index);
}

void ProgramChange::select_next() {
    Project& project = engine.session.get_project();
    Song& selected_song = project.get_songs().at(selected_song_index);
    if (selected_pattern_index >= selected_song.get_patterns().size() - 1) {
        if (selected_song_index >= project.get_songs().size() - 1) {
            // We're at the end
            return;
        }
        selected_song_index++;
        selected_pattern_index = 0;
    } else {
        selected_pattern_index++;   
    }
    update_display();
}

void ProgramChange::select_prev() {
    Project& project = engine.session.get_project();
    if (selected_pattern_index == 0) {
        if (selected_song_index == 0) {
            // We're at the beginning
            return;
        }
        selected_song_index--;
        selected_pattern_index = project.get_songs().at(selected_song_index).get_patterns().size() - 1;
    } else {
        selected_pattern_index--;        
    }
    update_display();
}

void ProgramChange::on_fader(float mix) {    
    // TODO:
}

void ProgramChange::on_program_changed() {
    selected_song_index = engine.session.get_song().get_number() - 1;
    selected_pattern_index = engine.session.get_pattern().get_number() - 1;
    update_display();
}