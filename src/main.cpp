#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "ostrostroj_app.hpp"

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