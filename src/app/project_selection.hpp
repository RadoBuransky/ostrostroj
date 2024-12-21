#pragma once
#include "workspace.hpp"
#include "display.hpp"
#include "alsamidi.hpp"
#include "project.hpp"

class ProjectSelectionScreen : public Screen {
    private:
        size_t project_count;
        int selected_project;
    public:
        ProjectSelectionScreen(size_t _project_count);
        virtual ~ProjectSelectionScreen() = default;
        virtual bool draw(unicorn_hat_mini_canvas& canvas);
        void set_selected_project(int index);
};

class ProjectSelection {
    private:
        Workspace &workspace;
        Display &display;
        AlsaMidi alsa_midi;
        ProjectSelectionScreen screen;
        int selected_project;
        std::atomic_flag done_flag;
        void set_selected_project(int index);
        void midi_callback();
    public:
        ProjectSelection(Workspace& _workspace, Display &_display);
        virtual ~ProjectSelection();
        Project& selectProject();
};