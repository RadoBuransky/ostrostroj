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

void Canvas::clear() {
    RGB value = palette_off;
    std::fill(canvas.begin(), canvas.end(), value);
}

void Canvas::point(size_t col, size_t row, RGB color) {
    canvas.at(col + (row * cols)) = color;
}