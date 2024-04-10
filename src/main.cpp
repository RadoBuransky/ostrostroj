#include <unistd.h>
#include <iostream>
#include <sys/reboot.h>
#include <string.h>
#include <signal.h>
#include <cstdlib>
#include <functional>
#include "spdlog/spdlog.h"
#include "soundcard.hpp"
#include "common.hpp"
#include "project.hpp"
#include "engine.hpp"

class OstrostrojApp {
    private:
        SoundCard soundCard;
        Project project;
        Engine engine;

        static void sigaction_handler(int s) {
            spdlog::info(std::format("Signal received [{}].", s));
        }

        void waitForSignal() const {
            struct sigaction sigIntHandler;
            sigIntHandler.sa_handler = sigaction_handler;
            sigemptyset(&sigIntHandler.sa_mask);
            sigIntHandler.sa_flags = 0;
            sigaction(SIGINT, &sigIntHandler, NULL);
            spdlog::info("Waiting...");
            pause();
        }

    public:
        OstrostrojApp():
            soundCard(SoundCard("ostrostroj")),
            project(Project("/home/ostrostroj/project/")),
            engine(Engine(project, soundCard)) {
            soundCard.start(std::bind(&Engine::next, &engine));
            project.verify(soundCard.get_sample_rate(), engine.get_loop_track_count());
        }

        virtual ~OstrostrojApp() {            
        }

        void main() const {
            waitForSignal();
        }
};

int main(int argc, char* argv[]) {
    spdlog::set_pattern("%L [%H:%M:%S] [%t] %v");
    spdlog::set_level(spdlog::level::debug);
    spdlog::info(std::format("Ostrostroj started. [{}]", static_cast<int>(spdlog::get_level())));
    try {
        if ((argc > 1) && (strcmp(argv[1], "shutdown") == 0)) {
            sync();
            reboot(RB_POWER_OFF); 
            spdlog::info("Shutdown!");
        } else {
            auto ostrostrojApp = OstrostrojApp();
            ostrostrojApp.main();
        }    
    } catch(std::exception const& e) {
        spdlog::error(e.what());
    }
    spdlog::info("Ostrostroj finished.");
}