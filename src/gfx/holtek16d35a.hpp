#pragma once

#include <gpiod.h>
#include <spidev_lib++.h>

constexpr uint8_t RGB_MAX = 0x3F;
constexpr uint8_t RGB_75 = 0x2E;
constexpr uint8_t RGB_50 = 0x1F;
constexpr uint8_t RGB_25 = 0x0F;
constexpr uint8_t RGB_10 = 0x0A;
constexpr uint8_t RGB_05 = 0x05;
constexpr uint8_t RGB_01 = 0x01;

struct RGB {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    RGB operator*(float c) const;
};

constexpr size_t HOLTEK_MAGIC_NUM = 28 * 8;

class Holtek16D35A {
    private:
        SPI spi;
        gpiod_line* cs_pin;
        size_t offset;
        std::array<uint8_t, 256> tx_buffer;
        SPI create_spi(std::string name);
        void write(size_t size);
    public:
        Holtek16D35A(std::string name, gpiod_line* _cs_pin, size_t _offset);
        virtual ~Holtek16D35A();

        /**
         * This command is set to initialise all functions. 
        */
        void soft_reset();
        /**
         * This command controls the 64-step PWM luminance control. It has a common setting for all dots.
        */
        void global_brightness(float level);
        /**
         * This command is used to control the scrolling on/off enable and scrolling type.
        */
        void scroll_ctrl();
        /**
         * This command controls the system oscillator on/off and display on/off.
        */
        void system_ctrl(uint8_t value);
        /**
         * This command is used to setup the COM pin output on/off status. 
        */
        void com_pin_ctrl(uint8_t value);
        /**
         * This command consists of four consecutive bytes to set up the ROW pin output on/off status.
        */
        void row_pin_ctrl(uint8_t value);
        uint8_t* get_display_data_buffer();
        void write_display_data();
        void shutdown();

};