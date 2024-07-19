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
static void sigaction_handler(int sig, siginfo_t*, void *) {
    SPDLOG_INFO("APP   sigaction[sig={}]", sig);
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

        void waitForSignal() {
            struct sigaction sigIntHandler;
            sigIntHandler.sa_sigaction = sigaction_handler;
            sigemptyset(&sigIntHandler.sa_mask);
            sigIntHandler.sa_flags = SA_SIGINFO;
            sigaction(SIGINT, &sigIntHandler, NULL);
            sigaction(SIGTERM, &sigIntHandler, NULL);
            sigaction(SIGKILL, &sigIntHandler, NULL);
            running_flag.test_and_set();
            SPDLOG_INFO("APP   running...");
            running_flag.wait(true);
        }

    public:
        OstrostrojApp(std::string workspace_dir):
            display(std::chrono::seconds(1)),
            alsa_pcm(),
            alsa_midi(),
            workspace(workspace_dir),
            engine(workspace, alsa_midi, alsa_pcm, display, running_flag) {
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
            SPDLOG_INFO("APP   finished.");
        }

        EngineExit main() {
            waitForSignal();
            return engine.get_exit_code();
        }
};

int main(int argc, char* argv[]) {
    spdlog::set_pattern("%L [%H:%M:%S.%e] [%t] %v");
    spdlog::set_level(spdlog::level::trace);
    SPDLOG_INFO("APP   started [{}]", static_cast<int>(spdlog::get_level()));
    int result = 1;
    try {
        if (argc < 2) {
            throw OstrostrojException("1 argument needed for workspace directory!");
        }        
        OstrostrojApp ostrostrojApp = OstrostrojApp(std::string(argv[1]));
        switch(ostrostrojApp.main()) {
            case EngineExit::ENGINE_EXIT_NOOP:
                result = 0;
                break;
            case EngineExit::ENGINE_EXIT_RESTART_SERVICE:
                result = 1; // Non-zero process result means failure which causes service to restart
                break;
            case EngineExit::ENGINE_EXIT_RESTART_DEVICE:            
                result = 2;
                break;
            case EngineExit::ENGINE_EXIT_SHUTDOWN_DEVICE:
                result = 3;
                break;
        }
    } catch (std::exception const &ex) {
        SPDLOG_ERROR("APP   failed[{}]", ex.what());
        result = 1;
    }
    spdlog::shutdown();
    return result;
}