#include "common.hpp"
#include <unistd.h>
#include <iostream>
#include <sys/reboot.h>
#include <signal.h>
#include <cstdlib>
#include <functional>
#include "project.hpp"
#include "engine.hpp"
#include "profiler.hpp"
#include "alsamidi.hpp"
#include "alsapcm.hpp"

class OstrostrojApp {
    private:
        AlsaPcm alsa_pcm;
        AlsaMidi alsa_midi;
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
            SPDLOG_INFO("App running...");
            pause();
        }

    public:
        OstrostrojApp():
            alsa_pcm(AlsaPcm()),
            alsa_midi(AlsaMidi()),
            project(Project("/home/rado/project/")),
            engine(Engine(project, alsa_midi, alsa_pcm)) {
            try {
                project.verify(alsa_pcm.get_sample_rate(), engine.get_loop_track_count());
                alsa_pcm.start(
                    std::bind(&Engine::pcm_event_callback, &engine, std::placeholders::_1, std::placeholders::_2),
                    std::bind(&Engine::pcm_callback, &engine, std::placeholders::_1));
                alsa_midi.start(std::bind(&Engine::midi_callback, &engine));
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
    spdlog::set_pattern("%L [%H:%M:%S.%e] [%t] %v");
    spdlog::set_level(spdlog::level::trace);
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