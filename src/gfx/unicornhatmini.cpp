#include "common.hpp"
#include <linux/spi/spidev.h>
#include <errno.h>
#include <gpiod.h>
#include "unicornhatmini.hpp"

// https://pinout.xyz/pinout/unicorn_hat_mini#
// https://github.com/pimoroni/unicornhatmini-python/blob/master/library/unicornhatmini/__init__.py
//   - https://github.com/doceme/py-spidev/blob/master/spidev_module.c
// https://www.kernel.org/doc/Documentation/spi/spi-summary
// https://forums.raspberrypi.com/viewtopic.php?t=364364


// Holtek 16D35A https://cdn.shopify.com/s/files/1/0174/1800/files/HT16D35A_Bv120.pdf?v=1587113912
constexpr u_char HOLTEK_CMD_SOFT_RESET = 0xCC;

class Holtek16D35A {
    public:
        Holtek16D35A();
        virtual ~Holtek16D35A();
};

SPI UnicornHatMini::create_spi(std::string dev) {
    spi_config_t spi_config;
    spi_config.mode = 0; // https://en.wikipedia.org/wiki/Serial_Peripheral_Interface#Mode_numbers
    spi_config.speed = 6000;
    spi_config.delay = 0;
    spi_config.bits_per_word = 8;
    SPI result = SPI(dev.c_str(), &spi_config);
    if (!result.begin()) {
        throw OstrostrojException(fmt::format("UHATM cannot open SPI! [{}]", dev));
    }
    return result;
}

UnicornHatMini::UnicornHatMini():
    spi0(create_spi("/dev/spidev0.0")),
    spi1(create_spi("/dev/spidev0.1")) {
    uint8_t msg;

    // Soft reset
    if (spi0.write(&msg, sizeof(msg)) < 0) {
        throw OstrostrojException(fmt::format("UHATM write failed! [0,errno={}]", strerror(errno)));
    }
    if (spi1.write(&msg, sizeof(msg)) < 0) {
        throw OstrostrojException(fmt::format("UHATM write failed! [1,errno={}]", strerror(errno)));
    }

    SPDLOG_INFO("UHATM initialized");
}

UnicornHatMini::~UnicornHatMini() {
}