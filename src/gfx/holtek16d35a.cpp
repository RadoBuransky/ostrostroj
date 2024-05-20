#include "common.hpp"
#include <linux/spi/spidev.h>
#include <errno.h>
#include <gpiod.h>
#include "holtek16d35a.hpp"

// https://pypi.org/project/spidev/
// https://github.com/solarsamuel/raspi5_blink_LED

// Holtek 16D35A https://cdn.shopify.com/s/files/1/0174/1800/files/HT16D35A_Bv120.pdf?v=1587113912

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
    spi_config.speed = 1000000;
    spi_config.delay = 0;
    spi_config.bits_per_word = 8;
    SPI result = SPI(name.c_str(), &spi_config);
    if (!result.begin()) {
        throw OstrostrojException(fmt::format("HLTEK cannot open SPI! [{}]", name));
    }
    return result;
}

Holtek16D35A::Holtek16D35A(std::string name, gpiod_line* _cs_pin, size_t _offset):
    spi(create_spi(name)),
    cs_pin(_cs_pin),
    offset(_offset) {
    soft_reset();
    global_brightness(0.1);
    scroll_ctrl();
    system_ctrl(0x00);
    com_pin_ctrl(0xFF);
    row_pin_ctrl(0xFF);
    system_ctrl(0x03);
    write_display_data();
    SPDLOG_DEBUG("HLTEK initialized [{}]", name);
}

Holtek16D35A::~Holtek16D35A() {
    if (cs_pin)  {
        gpiod_line_release(cs_pin);
        cs_pin = nullptr;
    }
}

void Holtek16D35A::soft_reset() {
    tx_buffer[0] = 0xCC;
    write(1);
}

void Holtek16D35A::global_brightness(float level) {
    tx_buffer[0] = 0x37;
    tx_buffer[1] = level * 0x40;
    write(2);
}

void Holtek16D35A::scroll_ctrl() {
    tx_buffer[0] = 0x20;
    tx_buffer[1] = 0x00;
    write(2);
}

void Holtek16D35A::system_ctrl(uint8_t value) {
    tx_buffer[0] = 0x35;
    tx_buffer[1] = value;
    write(2);
}

void Holtek16D35A::com_pin_ctrl(uint8_t value) {
    tx_buffer[0] = 0x41;
    tx_buffer[1] = value;
    write(2);
}

void Holtek16D35A::row_pin_ctrl(uint8_t value) {
    tx_buffer[0] = 0x42;
    tx_buffer[1] = value;
    tx_buffer[2] = value;
    tx_buffer[3] = value;
    tx_buffer[4] = value;
    write(5);
}

uint8_t* Holtek16D35A::get_display_data_buffer() {
    return tx_buffer.data() + 2;
}

void Holtek16D35A::write_display_data() {
    tx_buffer[0] = 0x80;
    tx_buffer[1] = 0x00;
    write(2 + HOLTEK_MAGIC_NUM);
}

void Holtek16D35A::shutdown() {    
    com_pin_ctrl(0x00);
    row_pin_ctrl(0x00);
    system_ctrl(0x00);
}