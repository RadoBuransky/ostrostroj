#include <unistd.h>
#include <iostream>
#include <sys/reboot.h>
#include <string.h>
#include "spdlog/spdlog.h"

int main(int argc, char* argv[]){
    if ((argc > 1) && (strcmp(argv[1], "shutdown") == 0)) {
        sync();
        reboot(RB_POWER_OFF); // Requires sudo
        std::cout << "Shutdown!\n";
    } else {
        spdlog::info("Welcome to spdlog!");
        spdlog::error("Some error message with arg: {}", 1);        
        spdlog::warn("Easy padding in numbers like {:08d}", 12);
        spdlog::critical("Support for int: {0:d};  hex: {0:x};  oct: {0:o}; bin: {0:b}", 42);
        
        std::cout << "Hello, from ostrostroj!\n";
    }
}
