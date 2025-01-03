#pragma once

#include "holtek16d35a.hpp"

constexpr RGB palette_off = {0, 0, 0};
constexpr RGB palette_red = {RGB_MAX, 0, 0};
constexpr RGB palette_green = {0, RGB_10, 0};
constexpr RGB palette_blue = {0, 0, RGB_01};
constexpr RGB palette_white = {RGB_MAX, RGB_25, RGB_25};
constexpr RGB palette_yellow = {RGB_MAX, RGB_10, 0};
constexpr RGB palette_magenta = {RGB_MAX, 0, RGB_01};
constexpr RGB palette_cyan = {0, RGB_05, RGB_05};

class Canvas {
    protected:
        const size_t cols;
        const size_t rows;
        std::vector<RGB> canvas;
    public:
        Canvas(size_t _cols, size_t _rows);
        virtual ~Canvas() = default;
        size_t get_cols() const;
        size_t get_rows() const;
        void clear();
        void point(size_t col, size_t row, RGB color);
        void hline(size_t start_col, size_t end_col, size_t row, RGB color);
        void vline(size_t col, size_t start_row, size_t end_row, RGB color);
};