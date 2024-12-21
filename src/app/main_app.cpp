#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "main_app.hpp"

static std::atomic_flag running_flag = ATOMIC_FLAG_INIT;
static void sigaction_handler(int sig, siginfo_t*, void *) {
    SPDLOG_INFO("APP   sigaction[sig={}]", sig);
    running_flag.clear();
    running_flag.notify_all();
}

void MainApp::waitForSignal() {
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

MainApp::MainApp(Project& _project, Display& _display):
    alsa_pcm(),
    engine(_project, alsa_pcm, _display, running_flag) {
    try {
        alsa_pcm.start(
            std::bind(&Engine::pcm_event_callback, &engine, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
            std::bind(&Engine::pcm_callback, &engine, std::placeholders::_1));
    } catch (std::exception const &ex) {
        SPDLOG_ERROR(ex.what());
        throw;
    }
}

MainApp::~MainApp() {
    engine.shutdown();
    SPDLOG_INFO("APP   finished.");
}

EngineExit MainApp::run() {
    waitForSignal();
    return engine.get_exit_code();
}