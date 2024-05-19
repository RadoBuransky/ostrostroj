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
#include "unicornhatmini.hpp"

static std::atomic_flag running_flag = ATOMIC_FLAG_INIT;
static void sigaction_handler(int) {
    running_flag.clear();
    running_flag.notify_all();
}

class OstrostrojApp {
    private:
        UnicornHatMini unicorn_hat_mini;
        AlsaPcm alsa_pcm;
        AlsaMidi alsa_midi;
        Project project;
        Engine engine;

        void waitForSignal() const {
            struct sigaction sigIntHandler;
            sigIntHandler.sa_handler = sigaction_handler;
            sigemptyset(&sigIntHandler.sa_mask);
            sigIntHandler.sa_flags = 0;
            sigaction(SIGINT, &sigIntHandler, NULL);
            running_flag.test_and_set();
            SPDLOG_INFO("Running...");
            running_flag.wait(true);
        }

    public:
        OstrostrojApp():
            unicorn_hat_mini(),
            alsa_pcm(AlsaPcm()),
            alsa_midi(AlsaMidi()),
            project(Project("/home/rado/project/")),
            engine(Engine(project, alsa_midi, alsa_pcm)) {
                /*
            try {
                project.verify(alsa_pcm.get_sample_rate(), engine.get_loop_track_count());
                alsa_pcm.start(
                    std::bind(&Engine::pcm_event_callback, &engine, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                    std::bind(&Engine::pcm_callback, &engine, std::placeholders::_1));
                alsa_midi.start(std::bind(&Engine::midi_callback, &engine));
            } catch (std::exception const &ex) {
                SPDLOG_ERROR(ex.what());
                throw;
            }
            */
        }

        virtual ~OstrostrojApp() {
            /*
            engine.shutdown();
            alsa_pcm.shutdown();
            alsa_midi.shutdown();
            */
            SPDLOG_INFO("Ostrostroj finished.");
        }

        void main() const {
            waitForSignal();
        }
};

int main(int argc, char* argv[]) {
    spdlog::set_pattern("%L [%H:%M:%S.%e] [%t] %v");
    spdlog::set_level(spdlog::level::trace);
    SPDLOG_INFO("Ostrostroj started. [{}]", static_cast<int>(spdlog::get_level()));
    try {
        if ((argc > 1) && (strcmp(argv[1], "shutdown") == 0)) {
            sync();
            reboot(RB_POWER_OFF); 
            SPDLOG_INFO("Shutdown!");
        } else {
            auto ostrostrojApp = OstrostrojApp();
            ostrostrojApp.main();
        }    
    } catch (std::exception const &ex) {
        SPDLOG_ERROR(ex.what());
    }
    spdlog::shutdown();
}