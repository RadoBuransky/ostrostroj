#include <unistd.h>
#include <iostream>
#include <sys/reboot.h>
#include <string.h>
#include "spdlog/spdlog.h"
#include "jack/jack.h"

class OstrostrojApp {
    public:
        OstrostrojApp() {            
        }

        void main() const {     
        }
};

int main(int argc, char* argv[]) {
    spdlog::info("Ostrostroj started.");
    try {
        if ((argc > 1) && (strcmp(argv[1], "shutdown") == 0)) {
            sync();
            reboot(RB_POWER_OFF); // TODO: Requires sudo, can we do this in shell script?
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