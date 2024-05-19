#pragma once

#include <gpiod.h>
#include "holtek16d35a.hpp"

constexpr size_t UNICORN_HAT_MINI_COLS = 17;
constexpr size_t UNICORN_HAT_MINI_ROWS = 7;

class UnicornHatMini {
    private:
        gpiod_chip* gpio;
        Holtek16D35A chip0;
        Holtek16D35A chip1;
        std::array<std::array<RGB, UNICORN_HAT_MINI_ROWS>, UNICORN_HAT_MINI_COLS> canvas;
        gpiod_chip* open_gpio(std::string name);
        gpiod_line* open_out_line(int pin_number);
        void show();
    public:
        UnicornHatMini();
        virtual ~UnicornHatMini();
};