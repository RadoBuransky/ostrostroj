#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "project_selection.hpp"

ProjectSelection::ProjectSelection(Workspace& _workspace, Display &_display, AlsaMidi &_alsa_midi):
    workspace(_workspace),
    display(_display),
    alsa_midi(_alsa_midi) {
}

ProjectSelection::~ProjectSelection() {
    alsa_midi.shutdown();
}

Project& ProjectSelection::selectProject() {
    // TODO: Implement
    return workspace.get_projects().back();
}