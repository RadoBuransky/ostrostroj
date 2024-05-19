#pragma once

#include <gpiod.h>
#include "holtek16d35a.hpp"

class UnicornHatMini {
    private:
        gpiod_chip* gpio;
        Holtek16D35A chip0;
        Holtek16D35A chip1;
        gpiod_chip* open_gpio(std::string name);
        gpiod_line* open_out_line(int pin_number);
    public:
        UnicornHatMini();
        virtual ~UnicornHatMini();
};