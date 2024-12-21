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
    engine(_project, _display, running_flag) {
}

MainApp::~MainApp() {
    engine.shutdown();
    SPDLOG_INFO("APP   finished.");
}

EngineExit MainApp::run() {
    waitForSignal();
    return engine.get_exit_code();
}