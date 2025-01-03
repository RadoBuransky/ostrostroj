#pragma once

#include "holtek16d35a.hpp"
#include "canvas.hpp"

constexpr size_t UNICORN_HAT_MINI_COLS = 17;
constexpr size_t UNICORN_HAT_MINI_ROWS = 7;

class UnicornHatMini : public Canvas {
    private:
        gpiod_chip* gpio;
        Holtek16D35A chip0;
        Holtek16D35A chip1;
        gpiod_chip* open_gpio(std::string name);
        gpiod_line* open_out_line(int pin_number);
    public:
        UnicornHatMini();
        virtual ~UnicornHatMini();
        void show();
};