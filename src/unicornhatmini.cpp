#include "common.hpp"
#include "unicornhatmini.hpp"


SPI UnicornHatMini::create_spi(std::string dev) {
    spi_config_t spi_config;
    spi_config.mode=0;
    spi_config.speed=1000000;
    spi_config.delay=0;
    spi_config.bits_per_word=8;
    SPI result = SPI(dev.c_str(), &spi_config);
    if (!result.begin()) {
        throw OstrostrojException(fmt::format("Cannot open SPI! [{}]", dev));
    }
    return result;
}

UnicornHatMini::UnicornHatMini():
    spi0(create_spi("/dev/spidev0.0")),
    spi1(create_spi("/dev/spidev0.1")) {
    SPDLOG_INFO("UHATM initialized");
}

UnicornHatMini::~UnicornHatMini() {
}