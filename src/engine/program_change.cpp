#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "program_change.hpp"
#include "session.hpp"
#include "engine.hpp"

ProgramChange::ProgramChange(Engine& _engine):
    engine(_engine),
    selected_song_index(engine.session.get_song().get_number() - 1),
    selected_pattern_index(engine.session.get_pattern().get_number() - 1),
    active_clip_players(),
    selected_clip_players(),
    fading(false) {
    update_display();
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

void ProgramChange::set_gain(float gain, std::vector<std::reference_wrapper<ClipPlayer>>& clip_players) {
    for (ClipPlayer& clip_player : clip_players) {
        clip_player.set_gain(gain);
    }
}

void ProgramChange::select_next() {
    if (fading) {
        return;
    }
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
    if (fading) {
        return;
    }
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
    Song& selected_song = engine.session.get_project().get_songs().at(selected_song_index);
    Pattern& selected_pattern = selected_song.get_patterns().at(selected_pattern_index);
    if (!fading) {
        if (mix != 0.0f) {
            return;
        }
        if ((selected_song_index == engine.session.get_song().get_number() - 1) &&
            (selected_pattern_index == engine.session.get_pattern().get_number() - 1)) {
            return;
        }
        selected_clip_players = engine.add_loop_clips(selected_pattern);
        set_gain(0.0, selected_clip_players);
        // TODO: Resume on the next (quarter note) MIDI clock
        fading = true;
        return;
    }
    if (mix == 1.0f) {
        engine.session.change_program(selected_pattern.get_bank_pattern());
        active_clip_players = selected_clip_players;
        engine.lock_worker_tracks();
        for (ClipPlayer& clip_player : selected_clip_players) {
            engine.remove_clip_player(clip_player.get_clip());
        }
        engine.unlock_worker_tracks();
        selected_clip_players.clear();
        set_gain(1.0f, active_clip_players);        
        fading = false;
        return;
    }
    set_gain(mix, selected_clip_players);
    set_gain(1.0 - mix, active_clip_players);
}

void ProgramChange::on_program_changed(bool running) {
    if (running) {
        return;
    }
    engine.lock_worker_tracks();
    engine.clear_loop_clips(running);
    Pattern& pattern = engine.session.get_pattern();
    active_clip_players = engine.add_loop_clips(pattern);
    engine.unlock_worker_tracks();
    selected_song_index = engine.session.get_song().get_number() - 1;
    selected_pattern_index = pattern.get_number() - 1;
    selected_clip_players.clear();
    fading = false;
    update_display();
}