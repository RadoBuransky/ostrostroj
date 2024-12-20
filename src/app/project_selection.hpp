#pragma once
#include "workspace.hpp"
#include "display.hpp"
#include "alsamidi.hpp"
#include "project.hpp"

class ProjectSelection {
    private:
        Workspace &workspace;
        Display &display;
        AlsaMidi &alsa_midi;
    public:
        ProjectSelection(Workspace& _workspace, Display &_display, AlsaMidi &_alsa_midi);
        virtual ~ProjectSelection();
        Project& selectProject();
};