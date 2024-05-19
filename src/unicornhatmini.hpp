#pragma once

#include <spidev_lib++.h>

class UnicornHatMini {
    private:
        SPI spi0;
        SPI spi1;
        SPI create_spi(std::string dev);
    public:
        UnicornHatMini();
        virtual ~UnicornHatMini();
};