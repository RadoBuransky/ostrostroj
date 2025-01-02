#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "ostrostroj_app.hpp"
#include "project_selection.hpp"
#include "main_app.hpp"

Project& OstrostrojApp::select_project() {
    std::unique_ptr<ProjectSelection> project_selection = std::make_unique<ProjectSelection>(workspace, display);
    return project_selection->selectProject();
}

EngineExit OstrostrojApp::main_app(Project& project) {
    std::unique_ptr<MainApp> main_app = std::make_unique<MainApp>(project, display);
    return main_app->run();
}

OstrostrojApp::OstrostrojApp(std::string workspace_dir):
    workspace(workspace_dir),
    display(std::chrono::milliseconds(100)) {            
}

EngineExit OstrostrojApp::main() {
    Project& project = select_project();
    return main_app(project);
}