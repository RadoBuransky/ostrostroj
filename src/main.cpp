#include "common.hpp"
#include <unistd.h>
#include <iostream>
#include <sys/reboot.h>
#include <signal.h>
#include <cstdlib>
#include <functional>
#include "soundcard.hpp"
#include "project.hpp"
#include "engine.hpp"
#include "profiler.hpp"

class OstrostrojApp {
    private:
        SoundCard soundCard;
        Project project;
        Engine engine;

        static void sigaction_handler(int s) {
            SPDLOG_INFO("Signal received [{}].", s);
        }

        void waitForSignal() const {
            struct sigaction sigIntHandler;
            sigIntHandler.sa_handler = sigaction_handler;
            sigemptyset(&sigIntHandler.sa_mask);
            sigIntHandler.sa_flags = 0;
            sigaction(SIGINT, &sigIntHandler, NULL);
            SPDLOG_INFO("Waiting...");
            pause();
        }

    public:
        OstrostrojApp():
            soundCard(SoundCard("ostrostroj")),
            project(Project("/home/ostrostroj/project/")),
            engine(Engine(project, soundCard)) {
            try {
                soundCard.start(std::bind(&Engine::next, &engine));
                project.verify(soundCard.get_sample_rate(), engine.get_loop_track_count());
            } catch (std::exception const &ex) {
                SPDLOG_ERROR(ex.what());
                throw;
            }
        }

        virtual ~OstrostrojApp() { 
            SPDLOG_INFO("Ostrostroj finished.");
            spdlog::shutdown();           
        }

        void main() const {
            waitForSignal();
        }
};

int main(int argc, char* argv[]) {
    spdlog::set_pattern("%L [%H:%M:%S] [%t] %v");
    spdlog::set_level(spdlog::level::info);
    SPDLOG_INFO("Ostrostroj started. [{}]", static_cast<int>(spdlog::get_level()));
    if ((argc > 1) && (strcmp(argv[1], "shutdown") == 0)) {
        sync();
        reboot(RB_POWER_OFF); 
        SPDLOG_INFO("Shutdown!");
    } else {
        auto ostrostrojApp = OstrostrojApp();
        try {
            ostrostrojApp.main();
        } catch (std::exception const &ex) {
            SPDLOG_ERROR(ex.what());
        }
    }
}