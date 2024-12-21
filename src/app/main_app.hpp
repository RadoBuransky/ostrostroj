#pragma once
#include "display.hpp"
#include "alsamidi.hpp"
#include "alsapcm.hpp"
#include "project.hpp"
#include "engine.hpp"

class MainApp {
    private:
        Engine engine;
        void waitForSignal();
    public:
        MainApp(Project& _project, Display& _display);
        virtual ~MainApp();
        EngineExit run();
};