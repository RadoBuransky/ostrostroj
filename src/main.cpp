#include <unistd.h>
#include <iostream>
#include <sys/reboot.h>
#include <string.h>

int main(int argc, char* argv[]){
    if ((argc > 1) && (strcmp(argv[1], "shutdown") == 0)) {
        sync();
        reboot(RB_POWER_OFF); // Requires sudo
        std::cout << "Shutdown!\n";
    } else {
        std::cout << "Hello, from ostrostroj!\n";
    }
}
