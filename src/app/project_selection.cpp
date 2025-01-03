#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "project_selection.hpp"

ProjectSelectionScreen::ProjectSelectionScreen(size_t _project_count):
    project_count(_project_count),
    selected_project(-1) {
}

bool ProjectSelectionScreen::draw(Canvas& canvas) {
    for (size_t i = 0; i < project_count; i++) {
        RGB color;
        if ((int) i == selected_project) {
            color = palette_red;
        } else {
            color = palette_blue;
        }
        canvas.point(i, 0, color);
    }
    return true;
}

void ProjectSelectionScreen::set_selected_project(int index) {
    selected_project = index;
}

void ProjectSelection::set_selected_project(int index) {
    if (index >= (int) workspace.get_projects().size()) {
        return;
    }
    if (index >= 0 && index == selected_project) {        
        selected_project = index;
        done_flag.test_and_set();
        done_flag.notify_all();
        return;
    }
    selected_project = index;
    screen.set_selected_project(index);
    display.tick(true);
}

void ProjectSelection::midi_callback() {
    snd_seq_event_t midi_event;
    if (!alsa_midi.get_fifo_in().pop(midi_event)) {
        return;
    }
    if (midi_event.type == SND_SEQ_EVENT_NOTEON) {
        size_t octave = midi_event.data.note.note / 12;
        if (octave == 4) {
            set_selected_project(midi_event.data.note.note % 12);
        }
    }
}

ProjectSelection::ProjectSelection(Workspace& _workspace, Display &_display):
    workspace(_workspace),
    display(_display),
    alsa_midi(),
    screen(workspace.get_projects().size()),
    selected_project(-1),
    done_flag(false) {
    display.set_active_screen(screen);
    alsa_midi.start(std::bind(&ProjectSelection::midi_callback, this));
}

ProjectSelection::~ProjectSelection() {
    alsa_midi.shutdown();
    display.set_active_screen(display.get_system_screen());
}

Project& ProjectSelection::selectProject() {
    if (workspace.get_projects().size() == 1) {
        return workspace.get_projects().at(0);
    }
    if (workspace.get_projects().size() > UNICORN_HAT_MINI_COLS) {
        throw OstrostrojException("WRKSP Too many projects in workspace!");
    }
    set_selected_project(-1);
    SPDLOG_INFO("WRKSP Waiting for project selection...");
    done_flag.wait(false);
    Project& result = workspace.get_projects().at(selected_project);
    SPDLOG_INFO("WRKSP Project selected [name={}]", result.get_name());
    return result;
}