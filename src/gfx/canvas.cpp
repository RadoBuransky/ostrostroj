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

uint8_t Canvas::get_last_col() const {
    return cols - 1;
}

size_t Canvas::get_rows() const {
    return rows;
}

uint8_t Canvas::get_last_row() const {
    return rows - 1;
}

void Canvas::clear() {
    RGB value = palette_off;
    std::fill(canvas.begin(), canvas.end(), value);
}

void Canvas::point(uint8_t col, uint8_t row, RGB color) {
    canvas.at(std::min(col, (uint8_t)(cols - 1)) + (std::min(row, (uint8_t)(rows - 1)) * cols)) = color;
}

void Canvas::line(uint8_t start_col, uint8_t end_col, uint8_t row, RGB color) {
    for (size_t col = start_col; col <= end_col; col++) {
        point(col, row, color);
    }
}

void Canvas::progress(uint8_t row, size_t count, uint8_t index, RGB color) {
    if (count == 0) {
        return;
    }
    point(0, row, index == 0 ? color : palette_blue);
    for (uint8_t i = 1; i < count; i++) {
        point(get_progress_point_col(count, i), row, i == index ? color : palette_blue);
    }
}

uint8_t Canvas::get_progress_point_col(size_t count, uint8_t index) {
    if (index == 0) {
        return 0;
    }
    return (cols-1)*index/(count-1);
}