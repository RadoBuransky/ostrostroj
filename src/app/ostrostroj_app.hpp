#pragma once
#include "workspace.hpp"
#include "display.hpp"
#include "alsamidi.hpp"
#include "engine.hpp"

class OstrostrojApp {
    private:
        Workspace workspace;
        Display display;
        Project& select_project();
        EngineExit main_app(Project& project);
    public:
        OstrostrojApp(std::string workspace_dir);
        virtual ~OstrostrojApp() = default;        
        EngineExit main();
};