#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 1
#include <spdlog/spdlog.h>
#include "program_change.hpp"
#include "session.hpp"
#include "engine.hpp"

ProgramChange::ProgramChange(Engine& _engine):
    engine(_engine),
    selected_song(engine.session.get_song()),
    selected_pattern(engine.session.get_pattern()),
    active_clip_players(),
    selected_clip_players(),
    fading(false),
    resume_selected_clip_players(false) {
    update_display();
}

void ProgramChange::update_display() {    
    MainScreen& main_screen = engine.display.get_main_screen();
    Project& project = engine.session.get_project();
    main_screen.set_song_count(project.get_songs().size());
    main_screen.set_pattern_count(selected_song.get().get_patterns().size());
    main_screen.set_active_song_index(engine.session.get_song().get_number() - 1);
    main_screen.set_selected_song_index(selected_song.get().get_number() - 1);
    main_screen.set_active_pattern_index(engine.session.get_pattern().get_number() - 1);
    main_screen.set_selected_pattern_index(selected_pattern.get().get_number() - 1);
    main_screen.set_pattern_fade(fading ? selected_clip_players.at(0).get().get_gain() : -1.0f);
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
    if (selected_pattern.get().get_number() >= selected_song.get().get_patterns().size()) {
        if (selected_song.get().get_number() >= project.get_songs().size()) {
            // We're at the end
            return;
        }
        selected_song = project.get_songs().at(selected_song.get().get_number());
        selected_pattern = selected_song.get().get_patterns().at(0);
    } else {
        selected_pattern = selected_song.get().get_patterns().at(selected_pattern.get().get_number());
    }
    update_display();
}

void ProgramChange::select_prev() {
    if (fading) {
        return;
    }
    Project& project = engine.session.get_project();
    if (selected_pattern.get().get_number() == 1) {
        if (selected_song.get().get_number() == 1) {
            // We're at the beginning
            return;
        }
        selected_song = project.get_songs().at(selected_song.get().get_number() - 2);
        selected_pattern = selected_song.get().get_patterns().back();
    } else {
        selected_pattern = selected_song.get().get_patterns().at(selected_pattern.get().get_number() - 2);        
    }
    update_display();
}

void ProgramChange::on_fader(float mix) {
    if (!fading) {
        if (mix != 0.0f) {
            return;
        }
        SPDLOG_DEBUG("PC    fading starting...");
        if ((selected_song.get().get_number() == engine.session.get_song().get_number()) &&
            (selected_pattern.get().get_number() == engine.session.get_pattern().get_number())) {
            return;
        }
        selected_clip_players = engine.add_loop_clips(selected_pattern);
        if (selected_clip_players.empty()) {
            return;
        }
        set_gain(0.0, selected_clip_players);
        fading = true;
        resume_selected_clip_players = true;
        SPDLOG_DEBUG("PC    fading started");
        return;
    }
    if (mix == 0.0f) {
        // TODO: Cancel        
    }
    if (mix == 1.0f) {
        SPDLOG_DEBUG("PC    fading ending...");
        engine.lock_worker_tracks();
        engine.session.change_program(selected_pattern.get().get_bank_pattern());
        active_clip_players = selected_clip_players;
        for (ClipPlayer& clip_player : selected_clip_players) {
            engine.remove_clip_player(clip_player.get_clip());
        }
        engine.unlock_worker_tracks();
        SPDLOG_DEBUG("PC    players removed...");
        selected_clip_players.clear();
        set_gain(1.0f, active_clip_players);     
        fading = false;
        update_display();
        SPDLOG_DEBUG("PC    fading done");
        return;
    }
    set_gain(mix, selected_clip_players);
    set_gain(1.0 - mix, active_clip_players);
    update_display();
    SPDLOG_DEBUG("PC    fading[mix={}]", mix);
}

void ProgramChange::on_program_changed(bool running) {
    SPDLOG_DEBUG("PC    on_program_changed[running={}]", running);
    if (running) {
        return;
    }
    engine.lock_worker_tracks();
    engine.clear_loop_clips(running);
    selected_pattern = engine.session.get_pattern();
    active_clip_players = engine.add_loop_clips(selected_pattern);
    engine.unlock_worker_tracks();
    selected_song = engine.session.get_song();
    selected_clip_players.clear();
    fading = false;
    update_display();
}

void ProgramChange::on_quarter_note_clock() {
    if (!resume_selected_clip_players) {
        return;
    }
    resume_selected_clip_players = false;
    for (ClipPlayer& clip_player : selected_clip_players) {
        clip_player.set_paused(false);
    }
}