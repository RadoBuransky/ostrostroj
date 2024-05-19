#include "common.hpp"
#include <linux/spi/spidev.h>
#include <errno.h>
#include <gpiod.h>
#include "holtek16d35a.hpp"

// https://pypi.org/project/spidev/
// https://github.com/solarsamuel/raspi5_blink_LED

// Holtek 16D35A https://cdn.shopify.com/s/files/1/0174/1800/files/HT16D35A_Bv120.pdf?v=1587113912
constexpr uint8_t HOLTEK_CMD_SOFT_RESET = 0xCC;

void Holtek16D35A::write(size_t size) {
    if (gpiod_line_set_value(cs_pin, 0) < 0) {
        throw OstrostrojException(fmt::format("UHATM gpiod_line_set_value(0) failed! [errno={}]", strerror(errno)));
    }
    if (spi.write(tx_buffer.data(), size) < 0) {
        throw OstrostrojException(fmt::format("UHATM write failed! [errno={}]", strerror(errno)));
    }
    if (gpiod_line_set_value(cs_pin, 1) < 0) {
        throw OstrostrojException(fmt::format("UHATM gpiod_line_set_value(1) failed! [errno={}]", strerror(errno)));
    }
}

SPI Holtek16D35A::create_spi(std::string name) {
    spi_config_t spi_config;
    spi_config.mode = 0; // https://en.wikipedia.org/wiki/Serial_Peripheral_Interface#Mode_numbers
    spi_config.speed = 6000;
    spi_config.delay = 0;
    spi_config.bits_per_word = 8;
    SPI result = SPI(name.c_str(), &spi_config);
    if (!result.begin()) {
        throw OstrostrojException(fmt::format("HLTEK cannot open SPI! [{}]", name));
    }
    return result;
}

Holtek16D35A::Holtek16D35A(std::string name, gpiod_line* _cs_pin):
    spi(create_spi(name)),
    cs_pin(_cs_pin) {
    SPDLOG_INFO("HLTEK initialized [{}]", name);
}

Holtek16D35A::~Holtek16D35A() {
    if (cs_pin)  {
        gpiod_line_release(cs_pin);
        cs_pin = nullptr;
    }
}

void Holtek16D35A::soft_reset() {
    tx_buffer[0] = HOLTEK_CMD_SOFT_RESET;
    write(1);
}