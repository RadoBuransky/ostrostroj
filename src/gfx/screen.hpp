#pragma once
#include "unicornhatmini.hpp"

constexpr RGB palette_off = {0, 0, 0};
constexpr RGB palette_red = {RGB_MAX, 0, 0};
constexpr RGB palette_green = {0, RGB_10, 0};
constexpr RGB palette_blue = {0, 0, RGB_01};
constexpr RGB palette_white = {RGB_MAX, RGB_25, RGB_25};
constexpr RGB palette_yellow = {RGB_MAX, RGB_10, 0};
constexpr RGB palette_magenta = {RGB_MAX, 0, RGB_01};
constexpr RGB palette_cyan = {0, RGB_05, RGB_05};

struct Point {
    uint8_t x;
    uint8_t y;
};

class Screen {
    public:
        virtual ~Screen() = default;
        virtual bool draw(unicorn_hat_mini_canvas& canvas) = 0;
};