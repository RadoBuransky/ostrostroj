#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include <algorithm>
#include "canvas.hpp"

Canvas::Canvas(size_t _cols, size_t _rows):
    cols(_cols),
    rows(_rows),
    canvas(cols * rows) {
    clear();
}

size_t Canvas::get_cols() const {
    return cols;
}

size_t Canvas::get_rows() const {
    return rows;
}

void Canvas::clear() {
    RGB value = palette_off;
    std::fill(canvas.begin(), canvas.end(), value);
}

void Canvas::point(size_t col, size_t row, RGB color) {
    canvas.at(col + (row * cols)) = color;
}

void Canvas::hline(size_t start_col, size_t end_col, size_t row, RGB color) {
    for (size_t col = start_col; col <= end_col; col++) {
        point(col, row, color);
    }
}

void Canvas::vline(size_t col, size_t start_row, size_t end_row, RGB color) {
    for (size_t row = start_row; row <= end_row; row++) {
        point(col, row, color);
    }    
}