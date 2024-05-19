#pragma once

#include <gpiod.h>
#include <spidev_lib++.h>

class Holtek16D35A {
    private:
        SPI spi;
        gpiod_line* cs_pin;
        std::array<uint8_t, 128> tx_buffer;
        std::array<uint8_t, 128> rx_buffer;
        SPI create_spi(std::string name);
        void write(size_t size);
    public:
        Holtek16D35A(std::string name, gpiod_line* _cs_pin);
        virtual ~Holtek16D35A();

        void soft_reset();
};