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
        uint8_t get_last_col() const;
        size_t get_rows() const;
        uint8_t get_last_row() const;
        void clear();
        void point(uint8_t col, uint8_t row, RGB color);
        void line(uint8_t start_col, uint8_t end_col, uint8_t row, RGB color);
        void progress(uint8_t row, size_t count, uint8_t index, RGB color);
        uint8_t get_progress_point_col(size_t count, uint8_t index);
};