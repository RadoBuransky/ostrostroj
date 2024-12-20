#pragma once
#include "display.hpp"
#include "alsamidi.hpp"
#include "alsapcm.hpp"
#include "project.hpp"
#include "engine.hpp"

class MainApp {
    private:
        AlsaPcm alsa_pcm;
        Engine engine;
        void waitForSignal();
    public:
        MainApp(Project& _project, Display& _display, AlsaMidi& _alsa_midi);
        virtual ~MainApp();
        EngineExit run();
};