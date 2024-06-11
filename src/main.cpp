#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "workspace.hpp"
#include "engine.hpp"
#include "profiler.hpp"
#include "alsamidi.hpp"
#include "alsapcm.hpp"
#include "display.hpp"

static std::atomic_flag running_flag = ATOMIC_FLAG_INIT;
static void sigaction_handler(int) {
    running_flag.clear();
    running_flag.notify_all();
}

class OstrostrojApp {
    private:
        Display display;
        AlsaPcm alsa_pcm;
        AlsaMidi alsa_midi;
        Workspace workspace;
        Engine engine;

        void waitForSignal() const {
            struct sigaction sigIntHandler;
            sigIntHandler.sa_handler = sigaction_handler;
            sigemptyset(&sigIntHandler.sa_mask);
            sigIntHandler.sa_flags = 0;
            sigaction(SIGINT, &sigIntHandler, NULL);
            sigaction(SIGTERM, &sigIntHandler, NULL);
            sigaction(SIGKILL, &sigIntHandler, NULL);
            running_flag.test_and_set();
            SPDLOG_INFO("Running...");
            running_flag.wait(true);
        }

    public:
        OstrostrojApp():
            display(std::chrono::seconds(1)),
            alsa_pcm(),
            alsa_midi(),
            workspace("/home/rado/projects/"),
            engine(workspace, alsa_midi, alsa_pcm, display) {
            try {
                alsa_pcm.start(
                    std::bind(&Engine::pcm_event_callback, &engine, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                    std::bind(&Engine::pcm_callback, &engine, std::placeholders::_1));
                alsa_midi.start(std::bind(&Engine::midi_callback, &engine));
            } catch (std::exception const &ex) {
                SPDLOG_ERROR(ex.what());
                throw;
            }
        }

        virtual ~OstrostrojApp() {
            engine.shutdown();
            alsa_pcm.shutdown();
            alsa_midi.shutdown();
            SPDLOG_INFO("Ostrostroj finished.");
        }

        void main() const {
            waitForSignal();
        }
};

int main(int argc, char* argv[]) {
    // TODO: Get workspace root directory as an argument
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