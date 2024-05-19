#include "common.hpp"
#include "unicornhatmini.hpp"

// https://pinout.xyz/pinout/unicorn_hat_mini#
// https://github.com/pimoroni/unicornhatmini-python/blob/master/library/unicornhatmini/__init__.py
//   - https://github.com/doceme/py-spidev/blob/master/spidev_module.c
// https://www.kernel.org/doc/Documentation/spi/spi-summary
// https://forums.raspberrypi.com/viewtopic.php?t=364364

gpiod_chip* UnicornHatMini::open_gpio(std::string name) {
    gpiod_chip* result = gpiod_chip_open(name.c_str());
    if (!result) {
        throw OstrostrojException(fmt::format("UHATM gpiod_chip_open failed! [{},err={}]", name, strerror(errno)));
    }
    return result;
}

gpiod_line* UnicornHatMini::open_out_line(int pin_number) {
    gpiod_line* result = gpiod_chip_get_line(gpio, pin_number);
    if (!result) {
        throw OstrostrojException(fmt::format("UHATM gpiod_chip_get_line failed! [offset={},err={}]", pin_number, strerror(errno)));
    }
    if (gpiod_line_request_output(result, fmt::format("ostrostroj{}", pin_number).c_str(), 0) < 0) {
        throw OstrostrojException(fmt::format("UHATM gpiod_line_request_output failed! [offset={},err={}]", pin_number, strerror(errno)));
    }
    return result;
}

UnicornHatMini::UnicornHatMini():
    gpio(open_gpio("/dev/gpiochip4")),
    chip0("/dev/spidev0.0", open_out_line(24)),
    chip1("/dev/spidev0.1", open_out_line(26)) {
    SPDLOG_INFO("UHATM initialized");
}

UnicornHatMini::~UnicornHatMini() {
    if (gpio) {
        gpiod_chip_close(gpio);
        gpio = nullptr;
    }
}